#!/usr/bin/env python3

import csv

def calc_and_conv(name, new_name):
    size_time = {}
    with open(name, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            size = int(row['Size'])
            time = float(row['Time(ns)'])
            if size not in size_time:
                size_time[size] = []
            size_time[size].append(time)
        for size in size_time:
            times = size_time[size]
            size_time[size] = sum(times) / len(times)
    with open(new_name, 'w') as f:
        writer = csv.DictWriter(f, fieldnames=['Size(KB)', 'Time(us)'])
        writer.writeheader()
        res = sorted(size_time.items())
        for size, time in res:
            writer.writerow({ 'Size(KB)': size // 1024, 'Time(us)': time })

calc_and_conv("phx.csv", "phx-snap-avg-us.csv")
