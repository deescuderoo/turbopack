#!/usr/bin/python

import os
entries = os.scandir('logs/')

logs_dir = "logs"

n = 13
s = 1000
d = 10
num_it = 1

dec = 2
rat = 2

d_max = 1
s_max = 6
n_values = [5, 13, 21, 29, 37]
# n_values = [29, 37, 45, 53, 61]
assert d_max < s_max

d_set = [10**i for i in range(0, d_max+1)]
s_values = { 10**d: [10**i for i in range(max(d+1, 3), s_max+1)] for d in range(0, d_max+1) }


def print_power(i):
    if (i < 1000): return str(int(i))
    if (i == 1000): return "1k"
    if (i == 10000): return "10k"
    if (i == 100000): return "100k"
    if (i == 1000000): return "1M"
    if (i == 10000000): return "10M"
    else: raise Exception("Error")  


def parse(n, s, d):
    
    w = s/d

    filename = logs_dir + f"/logs_experiment_{n}_{s}_{d}_0/party_0.log"
    flag = os.path.exists(filename)
    if (not flag):
        return [0, 0, 0, 1, 1]

    fi_prep = 0
    fd_prep = 0
    online = 0
    dn07_prep = 0
    dn07_online = 0

    it = 0
    while (True):
        try:
            filename = logs_dir + f'/logs_experiment_{n}_{s}_{d}_{it}/party_0.log'
            file = open(filename, "r")
            it += 1
        except:
            break

        for line in file:
            if line.startswith("fi_prep:"):
                fi_prep += int(''.join(filter(str.isdigit, line)))
            if line.startswith("fd_prep:"):
                fd_prep += int(''.join(filter(str.isdigit, line)))
            if line.startswith("online:"):
                online += int(''.join(filter(str.isdigit, line)))
            if line.startswith("atlas_prep:"):
                dn07_prep += int(''.join(filter(str.isdigit, line)))
            if line.startswith("atlas_online:"):
                dn07_online += int(''.join(filter(str.isdigit, line)))

    fi_prep = fi_prep/it
    fd_prep = fd_prep/it
    online = online/it
    dn07_prep = dn07_prep/it
    dn07_online = dn07_online/it

    print(f"Parsed n={n}, s={s}, d={d}, num_it={it}")

    return [fi_prep / 10**6, fd_prep / 10**6, online / 10**6, dn07_prep / 10**6, dn07_online / 10**6]

l = parse(n, s, d)

res = {}
fd = {}
fi = {}

for d in d_set:
    for s in s_values[d]:
        fd[(s, d)] = ""
        fi[(s, d)] = ""
        for n in n_values:
            res = parse(n, s, d)

            off_fd = res[0] + res[1]
            on_fd = res[2]
            off_fi = res[0]
            on_fi = res[1] + res[2]
            off_dn07 = res[3]
            on_dn07 = res[4]

            ratio_off_fd = off_fd / off_dn07
            ratio_on_fd = on_fd / on_dn07
            ratio_off_fi = off_fi / off_dn07
            ratio_on_fi = on_fi / on_dn07

            fd[(s, d)] += f" & {off_fd:.{dec}f} / {on_fd:.{dec}f} & " + "\\textbf{" + f"{ratio_off_fd:.{rat}f} / {ratio_on_fd:.{rat}f}" + "}"
            fi[(s, d)] += f" & {off_fi:.{dec}f} / {on_fi:.{dec}f} & " + "\\textbf{" + f"{ratio_off_fi:.{rat}f} / {ratio_on_fi:.{rat}f}" + "}"

string = ""

for d in d_set:
    # string += "\\multirow{" + str(2*len(s_values[d])) + "}{*}{" + print_power(d) + "}"
    for s in s_values[d]:
        string += " \\multirow{2}{*}{" + print_power(s/d) + "} & \\textbf{CD}" + fd[(s, d)] + "\\\\\n"
        string += " & \\textbf{CI}" + fi[(s, d)] + "\\\\ \n"
        if (s != s_values[d][-1]):
            string += "\\midrule \n"

print(string)
