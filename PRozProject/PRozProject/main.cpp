#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MSG_SLV 1
const int MAYOR_ID = 0;

enum class state {
	INIT_STATE,
	WAIT,
	GET_ORDER,
	WAIT_FOR_ORDER,
	GET_TOOLS,
	WAIT_FOR_PIN,
	WAIT_FOR_POISON,
	CRITICAL_SECTION_ORDER,
	CRITICAL_SECTION_PIN,
	CRITICAL_SECTION_POISON,
	FINISHED_JOB,
	GENERATE_ORDERS,
	WAIT_FOR_ORDERS
};

struct order
{
	int id;
	int hamsters;
};

order* generateOrders(int gnomes, int* ordersAmount);

int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);

	int rank, size;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	state mayorInitState = state::GENERATE_ORDERS;
	state gnomeInitState = state::WAIT;

	if (rank == 0)
	{
		int gnomes;
		int ordersAmount;
		int hamsters = 100;
		MPI_Comm_size(MPI_COMM_WORLD, &gnomes);
		state myState = mayorInitState;
		while (hamsters > 0)
		{

			if (myState == state::GENERATE_ORDERS)
			{
				order* orders = generateOrders(gnomes, &ordersAmount);
				for (int i = 1; i < gnomes + 1; i++)
				{
					MPI_Send(orders, (gnomes + 1) * sizeof(order), MPI_BYTE, i, 0, MPI_COMM_WORLD);
				}
				break;
			}

			if (myState == state::WAIT_FOR_ORDERS)
			{

			}
		}
		printf("XD");
	}
	else 
	{
		state myState = gnomeInitState;

		int gnomes;
		MPI_Comm_size(MPI_COMM_WORLD, &gnomes);

		bool* acknowledgementTable = new bool[gnomes];
		while (true) {

			if (myState == state::WAIT)
			{
				order* orders = new order[gnomes + 1];
				MPI_Status status;
				MPI_Recv(orders, gnomes + 1, MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				printf("XD");
			}

			if (myState == state::GET_ORDER)
			{

			}

			if (myState == state::WAIT_FOR_ORDER)
			{

			}

			if (myState == state::GET_TOOLS)
			{

			}

			if (myState == state::WAIT_FOR_PIN)
			{

			}

			if (myState == state::WAIT_FOR_POISON)
			{

			}

			if (myState == state::CRITICAL_SECTION_ORDER)
			{

			}

			if (myState == state::CRITICAL_SECTION_PIN)
			{

			}

			if (myState == state::CRITICAL_SECTION_POISON)
			{

			}
		}
	}
	MPI_Finalize();
}

order* generateOrders(int gnomes, int* ordersAmount)
{
	srand(time(NULL));
	*ordersAmount = rand() % gnomes + 1;
	order* orders = new order[*ordersAmount + 1];
	orders[0] = order{ 0, *ordersAmount };
	for (int i = 1; i < *ordersAmount + 1; i++)
	{
		int hamsters = rand() % 10;
		orders[i] = order{ i, 10 };
	}
	return orders;
}