import matplotlib.pyplot as plt
import csv

my_list = []

def order_and_add (my_list, increment, decrement, initial_dim):
    times   = []
    sizes   = []

    ind     = initial_dim
    i       = 0
    j       = 0

    times.append(0)
    sizes.append(initial_dim)
    while ((i < len(my_list[0])) or (j < len(my_list[1]))):
        if i >= len(my_list[0]):
            ind-= decrement
            times.append(my_list[1][j])
            sizes.append(ind)
            j+= 1
        elif j >= len(my_list[1]):
            ind+= increment
            times.append(my_list[0][i])
            sizes.append(ind)
            i+= 1
        else:   
            if ((my_list[0][i]) <= (my_list[1][j])):
                ind+= increment
                times.append(my_list[0][i])
                sizes.append(ind)
                i+= 1
            else:
                ind-= decrement
                times.append(my_list[1][j])
                sizes.append(ind)
                j+= 1
    
    return [times, sizes]

with open("plot.csv", newline='\n') as csvfile:
    csvreader = csv.reader(csvfile)

    for row in csvreader:
        my_list.append([float(x) for x in row])

    param = my_list[4]

    [times, sizes] = order_and_add(my_list[0:2],1,1, 0)
    [times1, sizes1] = order_and_add(my_list[2:4],param[3], param[4], param[2])


    ax1 = plt.subplot(211)
    ax1.plot(times, sizes, "-1")
    ax1.plot(times, [param[1] for i  in range(1, len(times)+1)], linestyle="dashed", color="red")
    ax1.plot(times, [param[0] for i  in range(1, len(times)+1)],  linestyle="dashed", color="red")
    ax1.set_ylabel('size queue')

    ax2 = plt.subplot(212, sharex=ax1)
    ax2.plot(times1, sizes1, "o", color='orange')
    ax2.set_ylabel('production time')
    plt.xlabel('times (ms)')
    ax1.grid(True, axis="x", linestyle='--')
    ax2.grid(True, axis="x", linestyle='--')
    plt.show()