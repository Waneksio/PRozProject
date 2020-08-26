#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <windows.h>

void sleep()
{
	int seconds = rand() % 10 + 1;
	Sleep(100 * seconds);
}
#else
#include <unistd.h>

void sleep()
{
	int seconds = rand() % 10 + 1;
	usleep(100000 * seconds);
}
#endif

#define MSG_SLV 1
const int MAYOR_ID = 0;

enum class State {
	HUNT,
	INIT_STATE,
	WAIT,
	GET_ORDER,
	WAIT_FOR_ORDER,
	GET_PIN,
	GET_POISON,
	WAIT_FOR_PIN,
	WAIT_FOR_POISON,
	CRITICAL_SECTION_ORDER,
	CRITICAL_SECTION_PIN,
	CRITICAL_SECTION_POISON,
	FINISHED_JOB,
	GENERATE_ORDERS,
	WAIT_FOR_ORDERS
};

enum class Signal {
	ACK_Z,
	REQ_Z,
	ACK_A,
	REQ_A,
	ACK_T,
	REQ_T
};

struct order
{
	int id;
	int hamsters;
};

struct Packet
{
	int id;
	int priority;
	Signal signal;
};

struct Gnome
{
	int id;
	int priority;
};

bool compareGnomes(Gnome gnome1, Gnome gnome2);

bool checkAcknowledgementTable(bool* tablePointer, int tableSize, int requiredAcknowledgements);

order* generateOrders(int gnomes, int* ordersAmount);

int main(int argc, char** argv)
{
	srand(time(NULL));
	MPI_Init(&argc, &argv);

	int rank, size;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	State mayorInitState = State::GENERATE_ORDERS;
	State gnomeInitState = State::WAIT;

	if (rank == 0)
	{
		int gnomes;
		int ordersAmount;
		int hamsters = 10000;
		gnomes = size - 1;
		bool *acknowledgements = new bool[gnomes + 1];
		State myState = mayorInitState;
		while (hamsters > 0)
		{
			if (myState == State::GENERATE_ORDERS)
			{
				order* orders = generateOrders(gnomes, &ordersAmount);
				printf("wygenerowano %d zleceń\n", ordersAmount);
				//std::cout << "wygenerowano " << ordersAmount << "zleceń\n";
				for (int i = 1; i < ordersAmount; i++)
				{
					hamsters -= orders[i].hamsters;
				}

				for (int i = 1; i < gnomes + 1; i++)
				{
					MPI_Send(orders, (gnomes + 1) * sizeof(order), MPI_BYTE, i, 0, MPI_COMM_WORLD);
				}
				myState = State::WAIT_FOR_ORDERS;
				continue;
			}

			if (myState == State::WAIT_FOR_ORDERS)
			{
				memset(acknowledgements, false, gnomes + 1);
				order orderBuffer;
				MPI_Status status;

				while (!checkAcknowledgementTable(acknowledgements, gnomes, ordersAmount))
				{
					MPI_Recv(&orderBuffer, sizeof(order), MPI_BYTE, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &status);
					acknowledgements[orderBuffer.id] = true;
				}

				int sum = 0;
				for (int i = 0; i < gnomes + 1; i++)
				{
					if (acknowledgements[i])
					{
						sum += 1;
					}
				}
				printf("%d skrzaty wykonały swoje zlecenie\n", ordersAmount, sum);
				//std::cout << ordersAmount << " skrzaty wykonaly swoje zlecenie\n";
				myState = State::GENERATE_ORDERS;
				continue;
			}
		}
		MPI_Finalize();
	}
	else 
	{
		State myState = gnomeInitState;
		int gnomes;
		MPI_Comm_size(MPI_COMM_WORLD, &gnomes);
		order myOrder;
		int acknowledgementCount;
		int pins = 2000;
		int poisons = 10000;
		int ordersCount;
		int myPriority = 0;
		bool* acknowledgementTable = new bool[gnomes];
		int* hamstersToKill = new int[gnomes];
		std::vector<Gnome> gnomesOrder = std::vector<Gnome>();
		order* orders = new order[gnomes + 1];

		while (true) {
			if (myState == State::HUNT)
			{
				sleep();
				printf("%d killing %d hamsters\n", rank, myOrder.hamsters);
				//std::cout << rank << " killing " << myOrder.hamsters << " hamsters\n";
				MPI_Send(&myOrder, sizeof(order), MPI_BYTE, 0, 2, MPI_COMM_WORLD);
				myState = State::WAIT;
				continue;
			}

			if (myState == State::WAIT)
			{
				MPI_Status status;
				MPI_Recv(orders, gnomes * sizeof(order), MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
				
				ordersCount = orders[0].hamsters;
				
				myState = State::GET_ORDER;
				continue;
			}

			if (myState == State::GET_ORDER)
			{
				acknowledgementCount = 0;
				memset(acknowledgementTable, 0, gnomes);
				Packet packet = Packet{ rank, myPriority, Signal::REQ_Z };

				for (int i = 1; i < gnomes; i++)
				{
					if (i != rank)
					{
						MPI_Send(&packet, sizeof(Packet), MPI_BYTE, i, 1, MPI_COMM_WORLD);
					}
				}
				myState = State::WAIT_FOR_ORDER;
				continue;
			}

			if (myState == State::WAIT_FOR_ORDER)
			{
				Packet packet;
				MPI_Status status;
				int elementsInTable = 1;
				gnomesOrder.clear();
				gnomesOrder.push_back(Gnome{ rank, myPriority });
				

				while (elementsInTable < gnomes - 1)
				{
					MPI_Recv(&packet, sizeof(Packet), MPI_BYTE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
					Gnome newGnome = Gnome{ packet.id, packet.priority };

					gnomesOrder.push_back(newGnome);
					elementsInTable += 1;
				}

				std::sort(gnomesOrder.begin(), gnomesOrder.end(), compareGnomes);

				int positionInQueue = 0;
				bool noOrder = false;
				for (Gnome gnome : gnomesOrder)
				{
					if (positionInQueue >= ordersCount)
					{
						noOrder = true;
						break;
					}
					if (gnome.id == rank)
					{
						myOrder = orders[positionInQueue];
						break;
					}
					else
					{
						positionInQueue++;
					}
				}

				if (noOrder)
				{
					printf("%d waiting for another orders set\n", rank);
					//std::cout << rank << " waiting for another orders set\n";
					myState = State::WAIT;
					continue;
				}

				printf("%d taking order %d\n", rank, myOrder.id);
				//std::cout << rank << " taking order " << myOrder.id << "\n";
				myPriority++;
				myState = State::HUNT;
				continue;
			}

			if (myState == State::GET_PIN)
			{
				acknowledgementCount = 0;
				memset(&acknowledgementTable, 0, gnomes);

				Packet packet = Packet{ rank, myPriority, Signal::REQ_Z };
				for (int i = 1; i < gnomes; i++)
				{
					if (i != rank)
						MPI_Send(&packet, sizeof(Packet), MPI_BYTE, i, 1, MPI_COMM_WORLD);
				}
				myState = State::WAIT_FOR_PIN;
				continue;
			}

			if (myState == State::WAIT_FOR_PIN)
			{
				int requiredAcknowledgements = pins - ordersCount;
				Packet packet;
				MPI_Status status;

				while (acknowledgementCount < requiredAcknowledgements)
				{
					MPI_Recv(&packet, sizeof(Packet), MPI_BYTE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);

					if (packet.signal == Signal::REQ_A)
					{
						if (packet.priority < myPriority || (packet.priority = myPriority && packet.id < rank))
						{
							pins -= 1;
							Packet response = Packet{ rank, myPriority, Signal::ACK_A };
							MPI_Send(&packet, sizeof(Packet), MPI_BYTE, packet.id, 1, MPI_COMM_WORLD);
						}
						else
						{
							if (!acknowledgementTable[packet.id - 1])
							{
								acknowledgementTable[packet.id - 1] = true;
								acknowledgementCount++;
							}
						}
					}
					else if (packet.signal == Signal::ACK_A)
					{
						if (!acknowledgementTable[packet.id - 1])
						{
							acknowledgementTable[packet.id - 1] = true;
							acknowledgementCount++;
						}
					}
				}
				myState = State::HUNT;
				continue;
			}

			if (myState == State::CRITICAL_SECTION_PIN)
			{
				printf("%d taking pin", rank);
				pins -= 1;
				myPriority += 1;
				myState = State::GET_POISON;
				continue;
			}

			if (myState == State::GET_POISON)
			{
				acknowledgementCount = 0;
				memset(&acknowledgementTable, 0, gnomes);

				Packet packet = Packet{ rank, myPriority, Signal::REQ_T };
				for (int i = 1; i < gnomes; i++)
				{
					if (i != rank)
						MPI_Send(&packet, sizeof(Packet), MPI_BYTE, i, 1, MPI_COMM_WORLD);
				}
				myState = State::WAIT_FOR_POISON;
				continue;
			}

			if (myState == State::WAIT_FOR_POISON)
			{
				int receivedAcknowledgments = poisons;
				for (Gnome gnome : gnomesOrder)
				{
					if (gnome.id != rank)
					{
						receivedAcknowledgments -= orders[gnome.id].hamsters;
					}
				}
				
				Packet packet;
				MPI_Status status;

				while ( receivedAcknowledgments < myOrder.hamsters)
				{
					MPI_Recv(&packet, sizeof(Packet), MPI_BYTE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);

					if (packet.signal == Signal::REQ_A)
					{
						if (packet.priority < myPriority || (packet.priority = myPriority && packet.id < rank))
						{
							int orderId = 0;
							for (int i = 0; i < gnomes; i++)
							{
								if (gnomesOrder[i].id == packet.id)
								{
									orderId = i;
								}
							}
							poisons -= orders[orderId].hamsters;
							Packet response = Packet{ rank, myPriority, Signal::ACK_A };
							MPI_Send(&packet, sizeof(Packet), MPI_BYTE, packet.id, 1, MPI_COMM_WORLD);
						}
						else
						{
							if (!acknowledgementTable[packet.id - 1])
							{
								acknowledgementTable[packet.id - 1] = true;
								receivedAcknowledgments += orders[packet.id].hamsters;
							}
						}
					}
					else if (packet.signal == Signal::ACK_A)
					{
						if (!acknowledgementTable[packet.id - 1])
						{
							acknowledgementTable[packet.id - 1] = true;
							receivedAcknowledgments += orders[packet.id].hamsters;
						}
					}
				}
				myState = State::CRITICAL_SECTION_POISON;
				continue;
			}

			if (myState == State::CRITICAL_SECTION_POISON)
			{
				printf("%d taking poisons\n", rank);
				poisons -= myOrder.hamsters;
				myState = State::HUNT;
				myPriority += 1;
				continue;
			}

			if (myState == State::CRITICAL_SECTION_ORDER)
			{

			}
		}
		MPI_Finalize();
	}
}

bool compareGnomes(Gnome gnome1, Gnome gnome2)
{
	if (gnome1.priority == gnome2.priority)
		return gnome1.id < gnome2.id;
	return gnome1.priority < gnome2.priority;
}

bool checkAcknowledgementTable(bool* tablePointer, int tableSize, int requiredAcknowledgements)
{
	int acknowledgements = 0;
	for (int i = 0; i < tableSize; i++)
	{
		if (tablePointer[i])
		{
			acknowledgements += 1;
		}
	}
	return (acknowledgements >= requiredAcknowledgements);
}

order* generateOrders(int gnomes, int* ordersAmount)
{
	*ordersAmount = rand() % gnomes + 1;
	order* orders = new order[*ordersAmount + 1];
	orders[0] = order{ 0, *ordersAmount };
	for (int i = 1; i < *ordersAmount + 1; i++)
	{
		int hamsters = 1 + rand() % 10;
		orders[i] = order{ i, hamsters };
	}
	return orders;
}