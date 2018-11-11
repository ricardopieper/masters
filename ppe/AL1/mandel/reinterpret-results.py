import csv
import itertools

executables = ['seq','pipeline', 'map', 'pipeline_with_farm', 'pipeline_with_map', 'pipeline_with_farm_map']
reader = csv.DictReader(open("benchmark_results.csv"), fieldnames=['model','parallelism','time','stdev'], )

results = sorted(list(reader)[1:], key = lambda x: int(x['parallelism']))

final = []

for threads, group in itertools.groupby(results, lambda x: x['parallelism']):
    line = {'threads': threads}
    for d in list(group):
        line[d['model']] = d['time']
    final.append(line)

columns = list(executables)
columns.insert(0, 'threads')

writer = csv.DictWriter(open("reinterpreted-results.csv", mode='w'), fieldnames=columns)
writer.writeheader()
writer.writerows(final)
  