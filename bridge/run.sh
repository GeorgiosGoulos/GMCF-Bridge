ROWS=2 # Number of processes per row
COLS=3 # Number of processes per column
TOTAL=$(($ROWS*$COLS)) # Total number of processes

# g++ test_bridge.cc bridge.cc

if mpic++ -pthread -std=c++11 -DMPI_TOPOLOGY_OPT -Wall -Werror main.cc System.cc Tile.cc Bridge.cc; then		# COMPILATION
	mpiexec -np $TOTAL ./a.out $ROWS $COLS		# EXECUTION
fi
