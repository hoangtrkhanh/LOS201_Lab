set pagination off
set debuginfod enabled off
set architecture arm

set logging file /home/khanh/embedded_lab3/logs/gdb_bug1.txt
set logging overwrite on
set logging enabled on

target remote localhost:1234

break main
break process_data
break create_message
break calculate

break buggy_app.c:9 if i == len

continue
printf "\n=== MAIN FUNCTION ===\n"
backtrace

continue
printf "\n=== PROCESS_DATA ===\n"
backtrace
print len
print data[0]@5

continue
printf "\n=== BUG 1: OUT OF BOUNDS ===\n"
print i
print len
print i == len
print &data[i]
backtrace
info registers pc sp lr

set logging enabled off
