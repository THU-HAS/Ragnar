import matplotlib.pyplot as plt
import numpy as np
import os
from matplotlib.ticker import MultipleLocator, AutoMinorLocator
def find_folders_startswith(dir, strpattern):
    if not dir.endswith(os.path.sep):
        dir += os.path.sep
    entries = os.listdir(dir)
    folders = [entry for entry in entries if os.path.isdir(os.path.join(dir, entry))]
    matched_folders = [folder for folder in folders if folder.startswith(strpattern)]
    return sorted(matched_folders)

def find_file_args(dir, str_1, str_2):
    files = os.listdir(dir)
    filtered_files = [f for f in files if f.startswith(str_1) and f.endswith(str_2)]
    middle_parts = [f[len(str_1):-len(str_2)] for f in filtered_files]
    return sorted(middle_parts)
    
def read_data(filename):
    t_l = [[],[]]
    y_l = [[],[]]
    q_l = [[],[]]
    r_l = [[],[]]
    with open(filename, 'r') as file:
        for line in file:
            t, n, y, q = line.strip().split()
            n = int(n)
            t_l[n].append(float(t))
            y_l[n].append(float(y))
            q_l[n].append(float(q))
            if(float(q)!=0):
                r_l[n].append(float(y)/float(q))
    
    return (t_l, y_l, q_l, r_l)

def process(configname, savedir, prange, yrange):

    batchlist = find_folders_startswith(configname, "res")
    for p in prange:
        paramlist_s = find_file_args(os.path.join(configname, batchlist[0]), "sherman_result_", "_%d"%p)
        
        paramlist = sorted(list(map(int, paramlist_s)))
        paramlist_s = list(map(str, paramlist))
        paramlist_draw = paramlist.copy()


        for batchname in batchlist:
            savepath = os.path.join(savedir, configname, batchname)
            if not os.path.exists(savepath):
                os.makedirs(savepath)
            # y_all = []
            # q_all = []
            # r_all = []
            # r_all_f = []
            r_mean_all = []
            r_10_all = []
            r_90_all = []
            for param in paramlist:
                filepath = os.path.join(configname, batchname, "sherman_result_%d_%d"%(param, p))
                # print(filepath)
                    
                t_l, y_l, q_l, r_l = read_data(filepath)

                r_ll = r_l[0]+r_l[1]
                l10 = np.percentile(r_ll,10)
                l90 = np.percentile(r_ll,90)
                ld = l90 - l10
                r_l_f = [x for x in r_ll if x >= l10 - 1.5*ld and x <= l90 + 1.5*ld]
                # r_all_f.append(r_l_f)

                r_mean = np.mean(r_l_f)
                r_10_all.append(l10)
                r_90_all.append(l90)
                r_mean_all.append(r_mean)

            fig = plt.figure(dpi=300, figsize=(12,4))
            ax = fig.subplots()
            ax.plot(paramlist_draw, r_10_all, ".k", markersize=1)
            ax.plot(paramlist_draw, r_90_all, ".k", markersize=1)
            ax.plot(paramlist_draw, r_mean_all, "r", linewidth=0.5)

            # plt.xticks([0,128,256,384,512,640,768,896,1024,1152])
            # if(len(paramlist_draw) < 20):
            xt = list(range(0,256,64))
            xl = [str(x) for x in range(0,256,64)]
            plt.xticks(xt, xl)
            ax.xaxis.set_major_locator(MultipleLocator(64))
            ax.yaxis.set_major_locator(MultipleLocator(500))
            ax.xaxis.set_minor_locator(MultipleLocator(16))
            ax.yaxis.set_minor_locator(MultipleLocator(100))
            ax.grid(which='major', linestyle='-', linewidth='0.5', color='gray')
            ax.grid(which='minor', linestyle=':', linewidth='0.5', color='lightgray')
            ax.minorticks_on()
            ax.set_xlabel("offset (Bytes)")
            ax.set_ylabel("delay (CPU Cycles)")
            # plt.ylim(yrange)
            plt.xlim([0,256])
            plt.grid(True,'both')
            plt.savefig(os.path.join(savepath, "figure_%d.png"%p))
            plt.close()
            with open(os.path.join(savepath, "data_%d.txt"%p), 'wt') as file:
                file.writelines("%s,%f,%f,%f\n"%(paramlist_s[i], r_mean_all[i], r_10_all[i], r_90_all[i]) for i in range(len(paramlist_draw)))

            # plt.figure(dpi=300)
            
            # # plt.hist(r_all_f[0], color="red", alpha=0.7, edgecolor='black')
            # # plt.hist(r_all_f[1], color="blue", alpha=0.7, edgecolor='black')
            # # plt.xlim([6000,8000])
            # plt.savefig(os.path.join(savepath, "distr.png"))
            # plt.close()

        



    # []

if __name__ == "__main__":
    
    savepath = "res000"
    process("s4", savepath, [0,64,128,173,192,256], [4500,7000])