import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
import numpy as np
import argparse

def gen_boxplots2(data_files_dir_cpu, data_files_dir_gpu, time_divisor, columns, colors, labels, subrange_modifiers):
    W=[2, 50, 100, 200]
    XZ=[100, 150, 200, 250, 300, 350, 400, 400]
    Y=[50, 100, 100, 200, 200, 300, 300, 350]

    fig, axes = plt.subplots(2, 2)#, sharey=arey))
    axes = axes.flatten()

    width = 0.1
    handles = [Patch(facecolor=color, edgecolor='black', label=label) for color, label in zip(colors, labels)]
    handles.append(Patch(facecolor='purple', edgecolor='black', label='Mean'))
    handles.append(Patch(facecolor='black', edgecolor='black', label='Median'))
    fig.legend(handles=handles, title="Legend", loc='upper center')#, bbox_to_anchor=(0.5, -0.05), ncol=3)

    for w, ax in zip(W, axes):
        xs = []
        ps = []
        p = 1
        for xz, y in zip(XZ, Y):
            file_template = f'w{w}_xz{xz}_y{y}.csv'
            meshing_samples_cpu = pd.read_csv(f'{data_files_dir_cpu}/ve001_meshing_samples_{file_template}')
            frame_samples_cpu = pd.read_csv(f'{data_files_dir_cpu}/ve001_frame_samples_{file_template}')
            samples_cpu = pd.concat([frame_samples_cpu, meshing_samples_cpu])
            meshing_samples_gpu = pd.read_csv(f'{data_files_dir_gpu}/ve001_meshing_samples_{file_template}')
            meshing_samples_gpu = meshing_samples_gpu[(meshing_samples_gpu['gpu_meshing_time_elapsed_ns'] < 180000000000) & \
                (meshing_samples_gpu['real_meshing_time_elapsed_ns'] < 180000000000)]
            frame_samples_gpu = pd.read_csv(f'{data_files_dir_gpu}/ve001_frame_samples_{file_template}')
            samples_gpu = pd.concat([frame_samples_gpu, meshing_samples_gpu], axis=1)
            samples_gpu.to_csv('/tmp/test.csv')
            
            boxes = []
            positions = []
            pos_offset = (-(len(columns)/2) + 1)*width - width/2
            for col, mod in zip(columns, subrange_modifiers):
                data_cpu = mod(samples_cpu[col].dropna()) if mod != None else samples_cpu[col].dropna()
                data_cpu = [v/time_divisor for v in data_cpu]
                data_gpu = mod(samples_gpu[col].dropna()) if mod != None else samples_gpu[col].dropna()
                data_gpu = [v/time_divisor for v in data_gpu]
                boxes.append(data_cpu)
                boxes.append(data_gpu)
                positions.append(p - pos_offset)
                pos_offset += width
                positions.append(p - pos_offset)
                pos_offset += width

            bxp = ax.boxplot(boxes, positions=positions, widths=0.1, patch_artist=True, showfliers=False,
                             showmeans=True,
                             meanline=True, meanprops={'color': 'purple', 'linewidth': 2,
                                                       'marker': 'o', 'markerfacecolor': 'purple', 'markeredgecolor': 'purple', 'markersize': 2}, 
                             medianprops={'color': 'black', 'linewidth': 2,
                                                       'marker': 's', 'markerfacecolor': 'black', 'markeredgecolor': 'black', 'markersize': 2})

            chunks_count = frame_samples_cpu['chunks_in_use'][0]
            xs.append(chunks_count)
            ps.append(p)

            for patch, color in zip(bxp['boxes'], colors):
                patch.set_facecolor(color)

            p += 1

        ax.set_xticks(ps)
        ax.set_xticklabels(xs)
        ax.set_title(f'Benchmark for data frequency {w}')
        ax.set_xlabel('Chunks count')
        ax.set_ylabel('Time [ms]')

def gen_boxplots(data_files_dir_cpu, data_files_dir_gpu, sharey, time_divisor):
    W=[2, 50, 100, 200]
    XZ=[100, 150, 200, 250, 300, 350, 400, 400]
    Y=[50, 100, 100, 200, 200, 300, 300, 350]

    fig, axes = plt.subplots(2, 2)#, sharey=arey))
    fig, axes = plt.subplots(2, 2)#, sharey=arey))
    axes = axes.flatten()

    width = 0.1
    colors = ['lightblue', 'lightgreen', 'salmon', 'blue', 'green', 'red']
    labels = ['frame times (CPU)', 'meshing times (CPU)', 'real meshing times (CPU)',
             'frame times (GPU)', 'meshing times (GPU)', 'real meshing times (GPU)']
    handles = [Patch(facecolor=color, edgecolor='black', label=label) for color, label in zip(colors, labels)]
    handles.append(Patch(facecolor='purple', edgecolor='black', label='Mean'))
    handles.append(Patch(facecolor='black', edgecolor='black', label='Median'))
    fig.legend(handles=handles, title="Legend", loc='upper center')#, bbox_to_anchor=(0.5, -0.05), ncol=3)
    for w, ax in zip(W, axes):
        xs = []
        ps = []
        p = 1
        for xz, y in zip(XZ, Y):
            file_template = f'w{w}_xz{xz}_y{y}.csv'
            meshing_samples_cpu = pd.read_csv(f'{data_files_dir_cpu}/ve001_meshing_samples_{file_template}')
            frame_samples_cpu = pd.read_csv(f'{data_files_dir_cpu}/ve001_frame_samples_{file_template}')
            meshing_samples_gpu = pd.read_csv(f'{data_files_dir_gpu}/ve001_meshing_samples_{file_template}')
            meshing_samples_gpu = meshing_samples_gpu[(meshing_samples_gpu['gpu_meshing_time_elapsed_ns'] < 180000000000) & \
                (meshing_samples_gpu['real_meshing_time_elapsed_ns'] < 180000000000)]
            frame_samples_gpu = pd.read_csv(f'{data_files_dir_gpu}/ve001_frame_samples_{file_template}')
             
            frame_times_cpu = frame_samples_cpu['gpu_frame_time_elapsed_ns'][-180:]
            meshing_times_cpu = meshing_samples_cpu['gpu_meshing_time_elapsed_ns']
            real_meshing_times_cpu = meshing_samples_cpu['real_meshing_time_elapsed_ns']
            frame_times_gpu = frame_samples_gpu['gpu_frame_time_elapsed_ns'][-180:]
            meshing_times_gpu = meshing_samples_gpu['gpu_meshing_time_elapsed_ns']
            real_meshing_times_gpu = meshing_samples_gpu['real_meshing_time_elapsed_ns']

            chunks_count = frame_samples_cpu['chunks_in_use'][0]
            xs.append(chunks_count)
            ps.append(p)

            frame_times_cpu = [v/time_divisor for v in frame_times_cpu]
            meshing_times_cpu = [v/time_divisor for v in meshing_times_cpu]
            real_meshing_times_cpu = [v/time_divisor for v in real_meshing_times_cpu]
            frame_times_gpu = [v/time_divisor for v in frame_times_gpu]
            meshing_times_gpu = [v/time_divisor for v in meshing_times_gpu]
            real_meshing_times_gpu = [v/time_divisor for v in real_meshing_times_gpu]
            
            boxes = [frame_times_cpu, meshing_times_cpu, real_meshing_times_cpu,
                     frame_times_gpu, meshing_times_gpu, real_meshing_times_gpu]

            positions = [p - 2*width - width/2, p - width - width/2, p - width/2, 
                         p + width/2, p + width + width/2, p + 2*width + width/2]

            bxp = ax.boxplot(boxes, positions=positions, widths=0.1, patch_artist=True, showfliers=False,
                             showmeans=True,
                             meanline=True, meanprops={'color': 'purple', 'linewidth': 2,
                                                       'marker': 'o', 'markerfacecolor': 'purple', 'markeredgecolor': 'purple', 'markersize': 2}, 
                             medianprops={'color': 'black', 'linewidth': 2,
                                                       'marker': 's', 'markerfacecolor': 'black', 'markeredgecolor': 'black', 'markersize': 2})
            for patch, color in zip(bxp['boxes'], colors):
                patch.set_facecolor(color)
            

            p += 1

        ax.set_xticks(ps)
        ax.set_xticklabels(xs)
        ax.set_title(f'Benchmark for data frequency {w}')
        ax.set_xlabel('Chunks count')
        ax.set_ylabel('Time [ms]')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ve001 benchmark plot generator')
    parser.add_argument('--data-files-dir-cpu', '-c', help='dir with csv files')
    parser.add_argument('--data-files-dir-gpu', '-g', help='dir with csv files')
    parser.add_argument('--time-divisor', '-t', type=float, help='time divisor')
    parser.add_argument('--meshing', '-m', action='store_true', help='if to plot meshing or frames')
    args = parser.parse_args()
    
    if args.meshing:
        gen_boxplots2(args.data_files_dir_cpu, args.data_files_dir_gpu, args.time_divisor,
                        ['gpu_meshing_time_elapsed_ns', 'real_meshing_time_elapsed_ns'],
                        ['lightgreen', 'green', 'salmon', 'red'],
                        ['meshing times (CPU)', 'meshing times (GPU)', 'real meshing times (CPU)', 'real meshing times (GPU)'],
                        [None, None])
    else:
        gen_boxplots2(args.data_files_dir_cpu, args.data_files_dir_gpu, args.time_divisor,
                        ['gpu_frame_time_elapsed_ns', 'gpu_frame_time_elapsed_ns'],
                        ['lightgreen', 'green', 'salmon', 'red'],
                        ['frame times (CPU)', 'frame times (GPU)', 'frame times during gen. (CPU)', 'frame times during gen. (GPU)'],
                        [(lambda data: data[-180:]), None])

    plt.tight_layout()
    plt.subplots_adjust(hspace=0.3, wspace=0.2, left=0.03, right=0.97)
    plt.legend()
    plt.show()
