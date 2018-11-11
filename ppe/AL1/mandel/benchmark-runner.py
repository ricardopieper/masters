import subprocess
import re
import csv
executables = ['pipeline']
thread_configs = range(1, 17)


def find_times(text):
    regex = 'Average on [0-9].*? experiments = ([0-9.]*) \\(ms\\) Std\\. Dev\\. ([0-9].*)'
    matches = list(enumerate(re.finditer(regex, text, re.MULTILINE)))
   
    return [matches[0][1].group(1), matches[0][1].group(2)]

def run_test(test, times, threads):
    process = subprocess.Popen(["./"+test, '1000', '30000', str(times), str(threads)], stdout=subprocess.PIPE)
    out, err = process.communicate()
    return find_times(out)

def run():
    writer = csv.DictWriter(open("benchmark_results.csv", mode='w'), fieldnames=['model','parallelism','time', 'stdev'])
    writer.writeheader()
       
    for e in executables:
        if (e in ['seq']):
            print "RUNNING seq WITH 1 THREAD ONCE"
            times = run_test(e, 5, 1)
            writer.writerow({'model':e,'parallelism':'1','time': times[0], 'stdev':times[1]})  
        else:
            for t in thread_configs:
                print "RUNNING "+e+" WITH "+str(t)+" THREADS"
                times = run_test(e, 5, t)
                writer.writerow({'model':e, 'parallelism':str(t), 'time': times[0], 'stdev':times[1]})
            

run()