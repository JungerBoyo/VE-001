import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse

def showPlot(title, x_label, y_label, labels, samples_sets, show_legend, axis):
    SAMPLE_SIZE = 60

    x_axis = range(SAMPLE_SIZE)

    for n in range(len(samples_sets)):
        sample_set = samples_sets[n]

        sample_set_means = []
        sample_set_std_deviations = []
        for i in range(SAMPLE_SIZE):
            mean = 0
            count = 0
            for j in range(i, len(sample_set), SAMPLE_SIZE):
                mean += sample_set[j]
                count += 1
            mean /= count

            variance = 0
            for j in range(i, len(sample_set), SAMPLE_SIZE):
                variance += (sample_set[j] - mean)*(sample_set[j] - mean)
            variance /= count

            std_dev = np.sqrt(variance)

            sample_set_means.append(mean)
            sample_set_std_deviations.append(std_dev)
        
        if show_legend:
            axis.errorbar(x_axis, y=sample_set_means, yerr=sample_set_std_deviations, label=labels[n])
        else:
            axis.errorbar(x_axis, y=sample_set_means, yerr=sample_set_std_deviations)

    axis.set_title(title)
    axis.set(xlabel=x_label, ylabel=y_label)
    if show_legend:
        axis.legend()

columns_count = 0

def getAxis(axes, i, number_of_plots):
    return axes[int(i%columns_count)] if number_of_plots <= columns_count else axes[int(i/columns_count), int(i%columns_count)]

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ve001 benchmark plot generator')
    
    parser.add_argument('--frame_timings', '-t', action='store_true', help='plot frame timings')
    parser.add_argument('--meshing_timings', '-m', action='store_true', help='plot meshing timings')
    parser.add_argument('--prims_samples_ratio', '-r', action='store_true', help='plot prims samples ratio')    
    parser.add_argument('--data_usage_server', '-s', action='store_true', help='plot server memory usage')
    parser.add_argument('--data_usage_client', '-c', action='store_true', help='plot client memory usage')
    parser.add_argument('--chunks_count', '-u', action='store_true', help='plot chunks count')
    parser.add_argument('--columns_count', '-k', help='columns count')
    parser.add_argument('--h_space', '-p', help='columns count')
    parser.add_argument('--w_space', '-w', help='columns count')

    meshing_samples_csv_file_path = 've001_meshing_samples.csv'
    frame_samples_csv_file_path = 've001_frame_samples.csv'

    meshing_samples_data = pd.read_csv(meshing_samples_csv_file_path)
    frame_samples_data = pd.read_csv(frame_samples_csv_file_path)

    app_args = parser.parse_args()

    columns_count = int(app_args.columns_count)

    number_of_plots = app_args.frame_timings * 2 + app_args.meshing_timings * 2 + app_args.prims_samples_ratio + app_args.data_usage_server + app_args.data_usage_client

    figure,axes = plt.subplots(int(number_of_plots/columns_count)+int(number_of_plots%columns_count), number_of_plots if columns_count > number_of_plots else columns_count)

    i = 0

    if app_args.frame_timings:
        showPlot('Czas klatki (serwer)', 'klatka', 'czas [ms]', [], [frame_samples_data['gpu_frame_time_elapsed_ns']/1_000_000], False, getAxis(axes, i, number_of_plots))
        i += 1
        showPlot('Czas klatki (klient)', 'klatka', 'czas [ms]', [], [frame_samples_data['cpu_frame_time_elapsed_ns']/1_000_000], False, getAxis(axes, i, number_of_plots))
        i += 1


    if app_args.meshing_timings:
        showPlot('Czasy siatkowania kawałków (same wykonanie)', 'kawałek', 'czas [ms]', [], [meshing_samples_data['gpu_meshing_time_elapsed_ns']/1_000_000], False, getAxis(axes, i, number_of_plots))
        i += 1
        showPlot('Czasy siatkowania kawałków (potwierdzono wykonanie)', 'kawałek', 'czas [ms]', [], [meshing_samples_data['real_meshing_time_elapsed_ns']/1_000_000], False, getAxis(axes, i, number_of_plots))
        i += 1
        
    if app_args.prims_samples_ratio:
        showPlot(
            "Stosunek wygenerowanych\nprymitywów do ilości\nwygenerowanych fragmentów",
            "klatka",
            "prymity/fragmenty",
            [],
            [frame_samples_data['prims_generated']/frame_samples_data['samples_passed']],
            False,
            getAxis(axes, i, number_of_plots)
        )
        i += 1
    
    if app_args.data_usage_server:
        showPlot(
            f"Zużycie pamięci (serwer)\nWielkość puli = {int(frame_samples_data['gpu_passive_memory_usage'][0]/1_000_000)}[MB]",
            "klatka",
            "pamięć w użyciu [MB]",
            ['Aktywne zużycie', 'Realne zużycie'],# 'Pasywne zużycie'],
            [frame_samples_data['gpu_active_memory_usage_in_use']/1_000_000,
            frame_samples_data['gpu_active_memory_usage_real']/1_000_000],
            True,
            getAxis(axes, i, number_of_plots)
        )
        i += 1
    
    if app_args.data_usage_client:
        showPlot(
            f"Zużycie pamięci (klient)\nWielkość puli = {int(frame_samples_data['cpu_passive_memory_usage'][0]/1_000_000)}[MB]",
            "klatka",
            "pamięć w użyciu [MB]",
            ['Aktywne zużycie'],
            [frame_samples_data['cpu_active_memory_usage']/1_000_000],
            True,
            getAxis(axes, i, number_of_plots)
        )
        i += 1
    if app_args.chunks_count:
        showPlot('Ilość użytych kawałków', 'klatka', 'ilość kawałków', [], [frame_samples_data['chunks_in_use']], False, getAxis(axes, i, number_of_plots))
        i += 1

    if float(app_args.h_space) != 0.0:
        plt.subplots_adjust(hspace=float(app_args.h_space))
    if float(app_args.w_space) != 0.0:
        plt.subplots_adjust(wspace=float(app_args.w_space))
    plt.show()