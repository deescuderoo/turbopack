#!/usr/bin/python

import os
entries = os.scandir('logs/')


n = 13
s = 1000
d = 10
num_it = 1

dec = 2
rat = 1

d_max = 3
s_max = 4
assert d_max < s_max

d_set = [10**i for i in range(1, d_max+1)]
s_values = { 10**d: [10**i for i in range(max(d+1, 3), s_max+1)] for d in range(1, d_max+1) }


def print_power(i):
    if (i < 1000): return str(i)
    if (i == 1000): return "1k"
    if (i == 10000): return "10k"
    if (i == 100000): return "100k"
    if (i == 1000000): return "1M"
    else: raise Exception("Error")  


def parse(n, s, d, num_it):
    
    fi_prep = 0
    fd_prep = 0
    online = 0
    atlas_prep = 0
    atlas_online = 0

    for it in range(num_it):
        filename = f'logs/logs_exp_comp_{n}_{s}_{d}_{it}/party_0.log'
        file = open(filename, "r")

        for line in file:
            if "fi_prep:" in line:
                fi_prep += int(''.join(filter(str.isdigit, line)))
            if "fd_prep:" in line:
                fd_prep += int(''.join(filter(str.isdigit, line)))
            if "online:" in line:
                online += int(''.join(filter(str.isdigit, line)))
            if "atlas_prep:" in line:
                atlas_prep += int(''.join(filter(str.isdigit, line)))
            if "atlas_online:" in line:
                atlas_online += int(''.join(filter(str.isdigit, line)))

    fi_prep = fi_prep/num_it
    fd_prep = fd_prep/num_it
    online = online/num_it
    atlas_prep = atlas_prep/num_it
    atlas_online = atlas_online/num_it

    return [fi_prep / 10**6, fd_prep / 10**6, online / 10**6, atlas_prep / 10**6, atlas_online / 10**6]

l = parse(n, s, d, num_it)

res = {}
fd = {}
fi = {}

for d in d_set:
    for s in s_values[d]:
        fd[(s, d)] = ""
        fi[(s, d)] = ""
        for n in [13, 21, 29, 37, 45]:
            res = parse(n, s, d, num_it)

            off_fd = res[0] + res[1]
            on_fd = res[2]
            off_fi = res[0]
            on_fi = res[1] + res[2]
            off_atlas = res[3]
            on_atlas = res[4]

            ratio_off_fd = off_fd / off_atlas
            ratio_on_fd = on_fd / on_atlas
            ratio_off_fi = off_fi / off_atlas
            ratio_on_fi = on_fi / on_atlas

            fd[(s, d)] += f" & {off_fd:.{dec}f} / {on_fd:.{dec}f} & {ratio_off_fd:.{rat}f} / {ratio_on_fd:.{rat}f}"
            fi[(s, d)] += f" & {off_fi:.{dec}f} / {on_fi:.{dec}f} & {ratio_off_fi:.{rat}f} / {ratio_on_fi:.{rat}f}"

string = ""

for d in d_set:
    string += "\\multirow{" + str(2*len(s_values[d])) + "}{*}{" + print_power(d) + "}"
    for s in s_values[d]:
        string += "& \\multirow{2}{*}{" + print_power(s) + "} & \\textbf{FD}" + fd[(s, d)] + "\\\\\n"
        string += "& & \\textbf{FI}" + fi[(s, d)] + "\\\\ \n \\cmidrule{3-13} \n"

print(string)
