import matplotlib.pyplot as plt
import csv

my_list = []


def order_and_add (my_list, increment, initial_dim):
    times   = []
    sizes   = []

    ind     = initial_dim
    i       = 0
    j       = 0

    while ((i < len(my_list[0])) or (j < len(my_list[1]))):
        if i >= len(my_list[0]):
            ind-= increment
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
                ind-= increment
                times.append(my_list[1][j])
                sizes.append(ind)
                j+= 1
    
    return [times, sizes]

with open("filename.csv", newline='\n') as csvfile:
    csvreader = csv.reader(csvfile)

    for row in csvreader:
        my_list.append([float(x) for x in row])

    [times, sizes] = order_and_add(my_list[0:2],1, 0)
    [times1, sizes1] = order_and_add(my_list[2:4], 500, 2500)

    ax1 = plt.subplot(211)
    ax1.plot(times, sizes)
    ax1.set_ylabel('size queue')

    ax2 = plt.subplot(212, sharex=ax1)
    ax2.plot(times1, sizes1, "o", color='orange')
    ax2.set_ylabel('producer time')
    plt.xlabel('times ms')
    plt.show()