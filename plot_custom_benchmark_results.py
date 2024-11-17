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

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ve001 benchmark plot generator')
    parser.add_argument('--csvfile', help='columns count')
    parser.add_argument('--titles', nargs='+', required=True)
    parser.add_argument('--xlabel', help='columns count')
    parser.add_argument('--ylabel', help='columns count')
    parser.add_argument('--divisor', help='columns count')
    parser.add_argument('--w_space', '-w', help='columns count')
    parser.add_argument('--sharey', '-y', help='columns count')
    app_args = parser.parse_args()
        
    samples = pd.read_csv(app_args.csvfile)

    _, axes = plt.subplots(1, len(samples.columns.tolist()), sharey=True if \
            int(app_args.sharey) == 1 else False)
    
    for col_name, title, axis in zip(samples.columns.tolist(), app_args.titles, axes):
        showPlot(title, app_args.xlabel, app_args.ylabel, [], \
                [samples[col_name]/int(app_args.divisor)], False, axis)

    if float(app_args.w_space) != 0.0:
        plt.subplots_adjust(wspace=float(app_args.w_space))

    plt.show()
    
