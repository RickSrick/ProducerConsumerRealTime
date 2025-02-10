PROGRAMS = src/prod_cons_queue \
           src/prod_cons_local 
		   
LDLIBS = -pthread

# NOTE:
# here we are using a hidden implicit rule that links any .c file into an executable
# to see all active implicit rules use: make -p 

all: $(PROGRAMS)

clean:
	rm $(PROGRAMS)