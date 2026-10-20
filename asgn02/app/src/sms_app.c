/* Learning reference: integrate sensor/processor/alert/logger threads. */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "sms_types.h"
#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"
#include "sms_ipc.h"

static volatile sig_atomic_t got_signal;
static void handle_signal(int sig) { (void)sig; got_signal = 1; }

typedef struct {
    mqd_t alarms;
    mqd_t logs;
    uint64_t dropped_alarms;
    uint64_t dropped_logs;
    double temp_threshold;
    double hum_threshold;
} QueueSink;

static int timed_send(mqd_t queue, const void *msg, size_t length)
{
    struct timespec deadline;
    if (clock_gettime(CLOCK_REALTIME, &deadline) < 0) return -errno;
    deadline.tv_sec += 2;
    int ret;
    do { ret = mq_timedsend(queue, msg, length, 0, &deadline); }
    while (ret == -1 && errno == EINTR);
    return ret == 0 ? 0 : -errno;
}

static void processor_sink(const SensorData *s, const SmsProcessorResult *r, void *user)
{
    QueueSink *sink = user;
    SmsLogMessage log = {.timestamp_ms=s->timestamp_ms, .kind=SMS_LOG_SAMPLE};
    snprintf(log.text, sizeof log.text,
             "T=%.2f H=%.2f avgT=%.2f avgH=%.2f deltaT=%.2f",
             s->temperature, s->humidity, r->avg_temperature,
             r->avg_humidity, r->temperature_change);
    if (timed_send(sink->logs, &log, sizeof log) != 0) sink->dropped_logs++;

    struct {unsigned int flag;uint32_t type;double value;double threshold;} a[] = {
        {SMS_ALERT_TEMP_HIGH,SMS_ALARM_TEMP_HIGH,s->temperature,sink->temp_threshold},
        {SMS_ALERT_HUMID_HIGH,SMS_ALARM_HUMID_HIGH,s->humidity,sink->hum_threshold},
        {SMS_ALERT_TEMP_SPIKE,SMS_ALARM_TEMP_SPIKE,r->temperature_change,5.0}
    };
    for (size_t i=0;i<3;i++) {
        if (!(r->alarm_flags & a[i].flag)) continue;
        SmsAlarmMessage alarm = {.timestamp_ms=s->timestamp_ms,
            .type=a[i].type,.value=a[i].value,.threshold=a[i].threshold};
        if (timed_send(sink->alarms,&alarm,sizeof alarm)!=0) sink->dropped_alarms++;
    }
}

static int parse_uint(const char *s, unsigned int *out)
{
    char *end;
    if (!s || !*s || *s=='-') return -1;
    errno=0; unsigned long x=strtoul(s,&end,10);
    if (errno || *end || x>UINT_MAX) return -1;
    *out=(unsigned int)x;return 0;
}
static int parse_double(const char *s,double *out)
{
    char *end;
    if (!s || !*s) return -1;
    errno=0;double x=strtod(s,&end);
    if (errno || *end || !isfinite(x)) return -1;
    *out=x;return 0;
}
static void usage(const char *name)
{
    printf("Usage: %s [-i ms] [-t C] [-h percent] [-d sec] [-v] [--mock] [--sched other|fifo] [--log path]\n",name);
}
static double elapsed_seconds(const struct timespec *start)
{
    struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
    return (double)(now.tv_sec-start->tv_sec)+1e-9*(now.tv_nsec-start->tv_nsec);
}
static int compare_jitter(const void *a,const void *b)
{
    int64_t x=*(const int64_t*)a,y=*(const int64_t*)b;
    return (x>y)-(x<y);
}
static void report_jitter(const SensorThreadContext *ctx, const char *mode)
{
    size_t n=ctx->jitter_count;
    if (n==0) {printf("JITTER mode=%s samples=0\n",mode);return;}
    int64_t *ordered=malloc(n*sizeof *ordered);
    if (!ordered) {fprintf(stderr,"Unable to summarize jitter\n");return;}
    memcpy(ordered,ctx->jitter_us,n*sizeof *ordered);
    qsort(ordered,n,sizeof *ordered,compare_jitter);
    long double sum=0;
    size_t hist[4]={0};
    for(size_t i=0;i<n;i++) {
        int64_t v=ctx->jitter_us[i];sum+=v;
        if (v<100)hist[0]++;
        else if (v<500)hist[1]++;
        else if (v<1000)hist[2]++;
        else hist[3]++;
    }
    size_t pos=(99*n+99)/100 - 1;
    printf("JITTER mode=%s n=%zu min_us=%" PRId64 " avg_us=%.2Lf max_us=%" PRId64 " p99_us=%" PRId64 "\n",
            mode,n,ordered[0],sum/n,ordered[n-1],ordered[pos]);
    printf("HIST lt100=%zu 100to499=%zu 500to999=%zu ge1000=%zu\n",
            hist[0],hist[1],hist[2],hist[3]);
    free(ordered);
}

int main(int argc,char **argv)
{
    SmsConfig cfg={.interval_ms=500,.temp_threshold=40,.hum_threshold=80,
                    .duration_sec=60,.verbose=0};
    bool mock=false,fifo=false;
    const char *log_path=SMS_DEFAULT_LOG_PATH;
    static struct option long_opts[]={{"mock",no_argument,NULL,1000},
       {"sched",required_argument,NULL,1001},{"log",required_argument,NULL,1002},
       {"help",no_argument,NULL,1003},{NULL,0,NULL,0}};
    int opt;
    while ((opt=getopt_long(argc,argv,"i:t:h:d:v",long_opts,NULL))!=-1) {
       switch(opt) {
       case 'i': if(parse_uint(optarg,&cfg.interval_ms)!=0 || cfg.interval_ms==0 || cfg.interval_ms>60000)goto bad_args;break;
       case 't': if(parse_double(optarg,&cfg.temp_threshold)!=0)goto bad_args;break;
       case 'h': if(parse_double(optarg,&cfg.hum_threshold)!=0)goto bad_args;break;
       case 'd': if(parse_uint(optarg,&cfg.duration_sec)!=0)goto bad_args;break;
       case 'v': cfg.verbose=1;break;
       case 1000:mock=true;break;
       case 1001:
           if (!strcmp(optarg,"fifo")) fifo=true;
           else if (!strcmp(optarg,"other")) fifo=false;
           else goto bad_args;
           break;
       case 1002: log_path=optarg;break;
       case 1003:usage(argv[0]);return 0;
       default:goto bad_args;
       }
    }
    if (optind!=argc || cfg.temp_threshold<=-273.15 || cfg.hum_threshold<0 || cfg.hum_threshold>100 || !*log_path)goto bad_args;

    struct sigaction sa={0};sa.sa_handler=handle_signal;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT,&sa,NULL)!=0 || sigaction(SIGTERM,&sa,NULL)!=0){perror("sigaction");return 1;}

    SmsBuffer buf={0};
    if(sms_buffer_init(&buf,SMS_BUFFER_CAPACITY)!=0){fprintf(stderr,"buffer init failed\n");return 1;}
    atomic_bool stop_requested=ATOMIC_VAR_INIT(false);
    SensorThreadContext sensor={.buffer=&buf,.config=&cfg,.stop_requested=&stop_requested,.use_mock=mock};
    sensor.jitter_capacity=20000;
    sensor.jitter_us=calloc(sensor.jitter_capacity,sizeof *sensor.jitter_us);
    if(!sensor.jitter_us){perror("jitter malloc");sms_buffer_destroy(&buf);return 1;}

    char alarm_name[64],log_name[64];
    snprintf(alarm_name,sizeof alarm_name,"/sms_a_%ld",(long)getpid());
    snprintf(log_name,sizeof log_name,"/sms_l_%ld",(long)getpid());
    struct mq_attr aa={.mq_maxmsg=10,.mq_msgsize=sizeof(SmsAlarmMessage)};
    struct mq_attr la={.mq_maxmsg=10,.mq_msgsize=sizeof(SmsLogMessage)};
    mqd_t aq=(mqd_t)-1,lq=(mqd_t)-1;
    bool logger_on=false,alert_on=false,processor_on=false,sensor_on=false,failed=false;
    pthread_t logger_tid,alert_tid,processor_tid,sensor_tid;
    SmsLoggerContext logger={.log_path=log_path};
    SmsAlertContext alert={.forward_logs=true};
    QueueSink sink={.temp_threshold=cfg.temp_threshold, .hum_threshold=cfg.hum_threshold};
    SmsProcessorContext processor={.buffer=&buf,.config=&cfg,.sink=processor_sink,.sink_user=&sink};

    aq=mq_open(alarm_name,O_CREAT|O_EXCL|O_RDWR,0600,&aa);
    if(aq==(mqd_t)-1){perror("mq_open alarm");failed=true;goto shutdown;}
    lq=mq_open(log_name,O_CREAT|O_EXCL|O_RDWR,0600,&la);
    if(lq==(mqd_t)-1){perror("mq_open log");failed=true;goto shutdown;}
    logger.log_queue=lq;alert.alarm_queue=aq;alert.log_queue=lq;
    sink.alarms=aq;sink.logs=lq;

    int ret=pthread_create(&logger_tid,NULL,sms_logger_thread,&logger);
    if(ret){fprintf(stderr,"start logger: %s\n",strerror(ret));failed=true;goto shutdown;}
    logger_on=true;
    ret=pthread_create(&alert_tid,NULL,sms_alert_thread,&alert);
    if(ret){fprintf(stderr,"start alert: %s\n",strerror(ret));failed=true;goto shutdown;}
    alert_on=true;
    ret=pthread_create(&processor_tid,NULL,sms_processor_thread,&processor);
    if(ret){fprintf(stderr,"start processor: %s\n",strerror(ret));failed=true;goto shutdown;}
    processor_on=true;

    pthread_attr_t attr;
    bool attr_inited=false;
    if(fifo) {
        if(mlockall(MCL_CURRENT|MCL_FUTURE)!=0) {
            perror("mlockall FIFO");failed=true;goto shutdown;
        }
        ret=pthread_attr_init(&attr);
        if(ret){fprintf(stderr,"pthread_attr_init: %s\n",strerror(ret));failed=true;goto shutdown;}
        attr_inited=true;
        struct sched_param priority={.sched_priority=50};
        if((ret=pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED)) ||
           (ret=pthread_attr_setschedpolicy(&attr,SCHED_FIFO)) ||
           (ret=pthread_attr_setschedparam(&attr,&priority))) {
            fprintf(stderr,"set sensor FIFO attr: %s\n",strerror(ret));
            pthread_attr_destroy(&attr);failed=true;goto shutdown;
        }
    }
    ret=pthread_create(&sensor_tid,attr_inited?&attr:NULL,sms_sensor_thread,&sensor);
    if(attr_inited)pthread_attr_destroy(&attr);
    if(ret){fprintf(stderr,"start sensor: %s\n",strerror(ret));failed=true;goto shutdown;}
    sensor_on=true;
    int actual_policy;
    struct sched_param actual_priority;
    ret = pthread_getschedparam(sensor_tid, &actual_policy, &actual_priority);
    if (ret != 0 || (fifo && (actual_policy != SCHED_FIFO || actual_priority.sched_priority != 50))) {
        fprintf(stderr,"Sensor scheduling verification failed\n");
        failed=true;
        goto shutdown;
    }
    printf("ACTUAL_SENSOR_SCHED policy=%s priority=%d\n",
           actual_policy == SCHED_FIFO ? "FIFO" : "OTHER",actual_priority.sched_priority);
    printf("START mode=%s scheduling=%s interval_ms=%u duration_sec=%u\n",
           mock?"mock":"device",fifo?"FIFO":"OTHER",cfg.interval_ms,cfg.duration_sec);
    struct timespec start;clock_gettime(CLOCK_MONOTONIC,&start);
    while(!got_signal && !atomic_load(&stop_requested)) {
        if(cfg.duration_sec && elapsed_seconds(&start)>=cfg.duration_sec)break;
        struct timespec pause={.tv_sec=0,.tv_nsec=100000000};
        while(nanosleep(&pause,&pause)<0 && errno==EINTR) {if(got_signal)break;}
    }

shutdown:
    atomic_store(&stop_requested,true);
    if(sensor_on)pthread_join(sensor_tid,NULL);
    sms_buffer_stop(&buf);
    if(processor_on)pthread_join(processor_tid,NULL);
    if(alert_on) {
        SmsAlarmMessage end={.type=SMS_ALARM_SHUTDOWN};
        if(timed_send(aq,&end,sizeof end)!=0){fprintf(stderr,"alert shutdown message failed\n");failed=true;pthread_cancel(alert_tid);}
        pthread_join(alert_tid,NULL);
    }
    if(logger_on) {
        SmsLogMessage end={.kind=SMS_LOG_SHUTDOWN};
        if(timed_send(lq,&end,sizeof end)!=0){fprintf(stderr,"logger shutdown message failed\n");failed=true;pthread_cancel(logger_tid);}
        pthread_join(logger_tid,NULL);
    }
    SmsBufferStats stats={0};sms_buffer_get_stats(&buf,&stats);
    printf("SUMMARY produced=%" PRIu64 " consumed=%" PRIu64 " dropped_buffer=%" PRIu64 " processed=%" PRIu64 " alerts=%" PRIu64 " temp=%" PRIu64 " hum=%" PRIu64 " spike=%" PRIu64 " logged=%" PRIu64 " sink_drop_alarm=%" PRIu64 " sink_drop_log=%" PRIu64 "\n",
         stats.total_produced,stats.total_consumed,stats.overflow_count,
         processor.processed,alert.total_alerts,alert.temp_high_count,
         alert.humid_high_count,alert.temp_spike_count,logger.records_written,
         sink.dropped_alarms,sink.dropped_logs);
    printf("SENSOR_DIAG eagain=%u read_errors=%u successful=%u\n",
           sensor.eagain_count,
           sensor.read_errors,
           sensor.samples_produced);
    if(sensor_on)report_jitter(&sensor,fifo?"FIFO":"OTHER");
    if(sensor.read_errors || processor.errors || alert.receive_errors || alert.forward_errors || logger.errors || sink.dropped_alarms || sink.dropped_logs || (!mock && sensor.samples_produced==0))failed=true;
    if(aq!=(mqd_t)-1){mq_close(aq);mq_unlink(alarm_name);}
    if(lq!=(mqd_t)-1){mq_close(lq);mq_unlink(log_name);}
    sms_buffer_destroy(&buf);
    free(sensor.jitter_us);
    if(fifo)munlockall();
    return failed?1:0;

bad_args:
    fprintf(stderr,"Invalid arguments\n");usage(argv[0]);return 2;
}
