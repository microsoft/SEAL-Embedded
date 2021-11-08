#!/usr/bin/python

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

import sys, getopt

def file_line_rep(filedata, base_string, min_val, max_val, set_val):
    #print("In file_line_rep. range: ["+str(min_val)+", "+str(max_val)+"], set_val: "+str(set_val))
    print("Setting "+base_string+" to "+str(set_val))

    if max_val < min_val:
        print("ERROR: max_val or min_val is set incorrectly")
        raise
    if set_val > max_val or set_val < min_val:
        print("ERROR: max_val or min_val or set_val is set incorrectly")
        raise

    newfiledata = []

    for line in filedata:
        #print(line)
        if line.find(base_string) != -1:
            #print(line)
            for i in range(min_val, max_val+1):
                line = line.replace(base_string+" "+str(i), base_string+" "+str(set_val))
                #print(line)
        newfiledata.append(line)

    #for line in newfiledata:
    #    print(line)

    return newfiledata

# -------------------------
#    Set the IFFT type
# -------------------------
def set_ifft_define(set_val):
    with open("lib/user_defines.h", "r") as file :
        filedata = file.readlines()

    newfiledata = file_line_rep(filedata, "#define SE_IFFT_TYPE", 0, 2, set_val)

    with open("lib/user_defines.h", "w") as file :
        file.writelines(newfiledata)


# ----------------------
#    Set the NTT type
# ----------------------
def set_ntt_define(set_val):
    with open("lib/user_defines.h", "r") as file :
        filedata = file.readlines()

    newfiledata = file_line_rep(filedata, "#define SE_NTT_TYPE", 0, 3, set_val)

    with open("lib/user_defines.h", "w") as file :
        file.writelines(newfiledata)


# -----------------------------
#    Set the Index Map type
# -----------------------------
def set_index_map_define(set_val):
    with open("lib/user_defines.h", "r") as file :
        filedata = file.readlines()

    newfiledata = file_line_rep(filedata, "#define SE_INDEX_MAP_TYPE", 0, 4, set_val)

    with open("lib/user_defines.h", "w") as file :
        file.writelines(newfiledata)

# -----------------------------
#    Set the SK type
# -----------------------------
def set_sk_define(set_val):
    with open("lib/user_defines.h", "r") as file :
        filedata = file.readlines()

    newfiledata = file_line_rep(filedata, "#define SE_SK_TYPE", 0, 2, set_val)

    with open("lib/user_defines.h", "w") as file :
        file.writelines(newfiledata)

# -----------------------------
#    Set the Data Load Type
# -----------------------------
def set_data_load_define(set_val):
    with open("lib/user_defines.h", "r") as file :
        filedata = file.readlines()

    newfiledata = file_line_rep(filedata, "#define SE_DATA_LOAD_TYPE", 0, 2, set_val)

    with open("lib/user_defines.h", "w") as file :
        file.writelines(newfiledata)

def main(argv):
    try:
       opts, args = getopt.getopt(argv,"hi:n:m:s:d:",["ifft=", "ntt=", "index_map=", "sk=", "data_load="])
    except getopt.GetoptError:
        print("change_defines.py -i <ifft_option> -n <ntt_option> "\
            "-m <index_map_option> -s <sk_option> -d <data_load_option>")
        sys.exit(2)

    ifft_set_val      = -1
    ntt_set_val       = -1
    index_map_set_val = -1
    sk_set_val        = -1
    data_load_set_val = -1

    for opt, arg in opts:
        if opt == '-h':
            print("change_defines.py -i <ifft_option> -n <ntt_option> "\
                "-m <index_map_option> -s <sk_option> -d <data_load_option>")
            sys.exit()
        elif opt in ("-i", "--ifft"):
            ifft_set_val = int(arg)
        elif opt in ("-n", "--ntt"):
            ntt_set_val = int(arg)
        elif opt in ("-m", "--index_map"):
            index_map_set_val = int(arg)
        elif opt in ("-s", "--sk"):
            sk_set_val = int(arg)
        elif opt in ("-d", "--data_load"):
            data_load_set_val = int(arg)

    if ifft_set_val < 0:
        print("Error: Need to choose an ifft option")
        sys.exit()
    elif ntt_set_val < 0:
        print("Error: Need to choose an ntt option")
        sys.exit()
    elif index_map_set_val < 0:
        print("Error: Need to choose an index_map option")
        sys.exit()
    elif sk_set_val < 0:
        print("Error: Need to choose an sk option")
        sys.exit()
    elif data_load_set_val < 0:
        print("Error: Need to choose a data load option")
        sys.exit()

    set_ifft_define(ifft_set_val)
    set_ntt_define(ntt_set_val)
    set_index_map_define(index_map_set_val)
    set_sk_define(sk_set_val)
    set_data_load_define(data_load_set_val)

if __name__ == '__main__':
    main(sys.argv[1:])
