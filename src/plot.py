import matplotlib.pyplot as plt
import csv

my_list = []
times   = []
sizes   = []

ind     = 0
i       = 0
j       = 0

with open("filename.csv", newline='\n') as csvfile:
    csvreader = csv.reader(csvfile)

    for row in csvreader:
        my_list.append([float(x) for x in row])


    while ((i < len(my_list[0])) or (j < len(my_list[1]))):
        
        if i >= len(my_list[0]):
            ind-= 1
            times.append(my_list[1][j])
            sizes.append(ind)
            j+= 1
        elif j >= len(my_list[1]):
            ind+= 1
            times.append(my_list[0][i])
            sizes.append(ind)
            i+= 1
        else:   
            if ((my_list[0][i]) <= (my_list[1][j])):
                ind+= 1
                times.append(my_list[0][i])
                sizes.append(ind)
                i+= 1
            else:
                ind-= 1
                times.append(my_list[1][j])
                sizes.append(ind)
                j+= 1


    print(times)
    print(sizes)
    plt.plot(times, sizes)
    plt.ylabel('size queue')
    plt.xlabel('times ms')
    plt.show()