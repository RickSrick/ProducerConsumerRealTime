PROGRAMS = prod_cons_queue \
           prod_cons_local 
		   
LDLIBS = -lpthread

# NOTE:
# here we are using a hidden implicit rule that links any .c file into an executable
# to see all active implicit rules use: make -p 

all: $(PROGRAMS)

clean:
	rm $(PROGRAMS)