#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <algorithm>
#include "Structures.h"
#include <iostream>
#include <map>
#include <thread>

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

std::map<Signal, State> signalState;
std::map<Signal, Signal> defaultResponse;

void initMap();

bool compareGnomes(Gnome* gnome1, Gnome* gnome2);

bool checkAcknowledgementTable(bool* tablePointer, int tableSize, int requiredAcknowledgements);

order* generateOrders(int gnomes, int* ordersAmount);

void wait(GnomeData* gnomeData);

void getOrder(GnomeData* gnomeData);

void wairForOrder(GnomeData* gnomeData, bool* syn);

void getPin(GnomeData* gnomeData);

void waitForPin(GnomeData* gnomeData);

void criticalSectionPin(GnomeData* gnomeData);

void hunt(GnomeData* gnomeData);

void sendToAll(Packet packet, int tag, int size, int rank);

void showGnomeData(GnomeData* gnomeData);

void processGnome(GnomeData* gnomeData, bool* syn);

void gnomeMonitor(GnomeData* gnomeData);

int main(int argc, char** argv)
{
	initMap();
	srand(time(NULL));
	//MPI_Init(&argc, &argv);
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

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
				for (int i = 1; i < ordersAmount; i++)
				{
					hamsters -= orders[i].hamsters;
				}

				for (int i = 1; i < gnomes + 1; i++)
				{
					MPI_Send(orders, (gnomes + 1) * sizeof(order), MPI_BYTE, i, 0, MPI_COMM_WORLD);
				}
				myState = State::WAIT_FOR_ORDERS;
				//break;
				continue;
			}

			if (myState == State::WAIT_FOR_ORDERS)
			{
				memset(acknowledgements, false, gnomes + 1);
				order orderBuffer;
				MPI_Status status;

				while (!checkAcknowledgementTable(acknowledgements, gnomes + 1, ordersAmount))
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
				printf("%d skrzaty wykonały swoje zlecenie\n", ordersAmount);
				printf("there is %d hamsters left\n", hamsters);
				myState = State::GENERATE_ORDERS;
				continue;
			}
		}
		MPI_Finalize();
	}
	else
	{
		GnomeData myData;
		myData.pins = 2000;
		myData.poisons = 10000;
		MPI_Comm_size(MPI_COMM_WORLD, &myData.gnomes);
		MPI_Comm_rank(MPI_COMM_WORLD, &myData.rank);
		myData.myState = gnomeInitState;
		myData.myPriority = 0;
		myData.acknowledgementTable = new bool[myData.gnomes];
		myData.hamstersToKill = new int[myData.gnomes];
		myData.gnomesOrder = std::vector<Gnome*>();
		myData.orders = new order[myData.gnomes + 1];
		memset(myData.acknowledgementTable, false, myData.gnomes);
		myData.receivedSignals = 0;
		myData.acknowledgementCount = 0;

		bool syn = false;

		/*std::thread sayHello(showGnomeData, &myData);

		while (true)
		{
			sayHello.join();
			std::thread sayHello(showGnomeData, &myData);
		}*/
		while (true)
		{
			std::thread monitorThread(processGnome, &myData, &syn);
			monitorThread.join();
			std::thread processThread(gnomeMonitor, &myData);
			processThread.join();

		}
		/*if (myState == State::GET_POISON)
		{
			int receivedAcknowledgments = poisons;
			for (int i = 0; i < ordersCount; i++)
			{
				if (gnomesOrder[i].id != rank)
				{
					receivedAcknowledgments -= orders[i].hamsters;
				}
			}
			if (receivedAcknowledgments > myOrder.hamsters)
			{
				myState = State::CRITICAL_SECTION_POISON;
				continue;
			}

			acknowledgementCount = 0;
			memset(&acknowledgementTable, 0, gnomes);

			Packet packet = Packet{ rank, myPriority, Signal::REQ_T };
			for (int i = 1; i < gnomes; i++)
			{
				if (i != rank)
				{
					MPI_Send(&packet, sizeof(Packet), MPI_BYTE, i, 1, MPI_COMM_WORLD);
				}
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
				int orderId = 0;
				for (int i = 0; i < ordersCount; i++)
				{
					if (gnomesOrder[i].id == packet.id)
					{
						orderId = i;
					}
				}

				if (packet.signal == Signal::REQ_T)
				{
					if (packet.priority < myPriority || (packet.priority = myPriority && packet.id < rank))
					{
						//poisons -= orders[orderId].hamsters;
						//Packet response = Packet{ rank, myPriority, Signal::ACK_A };
						//MPI_Send(&packet, sizeof(Packet), MPI_BYTE, packet.id, 1, MPI_COMM_WORLD);
					}
					else
					{
						if (!acknowledgementTable[packet.id - 1])
						{
							acknowledgementTable[packet.id - 1] = true;
							receivedAcknowledgments += orders[orderId].hamsters;
						}
					}
				}
				else if (packet.signal == Signal::ACK_T)
				{
					if (!acknowledgementTable[packet.id - 1])
					{
						acknowledgementTable[packet.id - 1] = true;
						receivedAcknowledgments += orders[orderId].hamsters;
					}
				}
			}
			myState = State::CRITICAL_SECTION_POISON;
			continue;
		}

		if (myState == State::CRITICAL_SECTION_POISON)
		{
			printf("%d taking %d poisons\n", rank, myOrder.hamsters);
			poisons -= myOrder.hamsters;
			myState = State::HUNT;
			myPriority += 1;
			continue;
		}

		if (myState == State::CRITICAL_SECTION_ORDER)
		{

		}
	}*/
		MPI_Finalize();
	}
}

void initMap()
{
	signalState[Signal::ACK_A] = State::WAIT_FOR_PIN;
	signalState[Signal::REQ_A] = State::WAIT_FOR_PIN;
	signalState[Signal::ACK_T] = State::WAIT_FOR_POISON;
	signalState[Signal::REQ_T] = State::WAIT_FOR_POISON;
	signalState[Signal::ACK_Z] = State::WAIT_FOR_ORDER;
	signalState[Signal::REQ_Z] = State::WAIT_FOR_ORDER;
	defaultResponse[Signal::REQ_A] = Signal::ACK_A;
	defaultResponse[Signal::REQ_T] = Signal::ACK_T;
	defaultResponse[Signal::REQ_Z] = Signal::ACK_Z;
}

bool compareGnomes(Gnome* gnome1, Gnome* gnome2)
{
	if (gnome1->priority == gnome2->priority)
		return gnome1->id < gnome2->id;
	return gnome1->priority < gnome2->priority;
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

void wait(GnomeData* gnomeData)
{
	MPI_Status status;
	MPI_Recv(gnomeData->orders, gnomeData->gnomes * sizeof(order), MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);

	gnomeData->ordersCount = gnomeData->orders[0].hamsters;
	gnomeData->myState = State::GET_ORDER;
	return;
}

void getOrder(GnomeData* gnomeData)
{
	Packet packet = Packet{ gnomeData->rank, gnomeData->myPriority, Signal::REQ_Z };
	gnomeData->myState = State::WAIT_FOR_ORDER;
	for (int i = 1; i < gnomeData->gnomes; i++)
	{
		if (i != gnomeData->rank)
		{
			MPI_Send(&packet, sizeof(Packet), MPI_BYTE, i, 1, MPI_COMM_WORLD);
		}
	}
}

void wairForOrder(GnomeData* gnomeData, bool* syn)
{
	/*Packet packet;
	MPI_Status status;
	int elementsInTable = 1;
	gnomeData->gnomesOrder.clear();
	gnomeData->gnomesOrder.push_back(new Gnome{ gnomeData->rank, gnomeData->myPriority });


	while (elementsInTable < gnomeData->gnomes - 1)
	{
		MPI_Recv(&packet, sizeof(Packet), MPI_BYTE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
		Gnome* newGnome = new Gnome{ packet.id, packet.priority };

		gnomeData->gnomesOrder.push_back(newGnome);
		elementsInTable += 1;
	}

	std::sort(gnomeData->gnomesOrder.begin(), gnomeData->gnomesOrder.end(), compareGnomes);

	int positionInQueue = 1;
	bool noOrder = true;
	for (Gnome* gnome : gnomeData->gnomesOrder)
	{
		if (positionInQueue >= gnomeData->ordersCount + 1)
		{
			break;
		}
		if (gnome->id == gnomeData->rank)
		{
			gnomeData->myOrder = gnomeData->orders[positionInQueue];
			noOrder = false;
			break;
		}
		else
		{
			positionInQueue += 1;
		}
	}
	printf("%d\n", positionInQueue);
	*/
	if (gnomeData->receivedSignals == gnomeData->gnomes - 2)
	{
		bool noOrder = true;
		int positionInQueue = gnomeData->gnomes - gnomeData->acknowledgementCount - 1;
		if (positionInQueue < gnomeData->ordersCount + 1)
		{
			noOrder = false;
			gnomeData->myOrder = gnomeData->orders[positionInQueue];
		}
		if (noOrder)
		{
			printf("%d waiting for another orders set\n", gnomeData->rank);
			gnomeData->myState = State::WAIT;
		}
		else
		{
			printf("%d taking order %d\n", gnomeData->rank, gnomeData->myOrder.id);
			gnomeData->myPriority += 1;
			gnomeData->myState = State::HUNT;
		}
		memset(gnomeData->acknowledgementTable, 0, gnomeData->gnomes);
		gnomeData->receivedSignals = 0;
		gnomeData->acknowledgementCount = 0;
	}
}

void getPin(GnomeData* gnomeData)
{
	if (gnomeData->pins - gnomeData->ordersCount + 1 > 0)
	{
		gnomeData->myState = State::WAIT_FOR_PIN;
		return;
	}
	gnomeData->acknowledgementCount = 0;
	memset(&gnomeData->acknowledgementTable, false, gnomeData->gnomes);

	Packet packet = Packet{ gnomeData->rank, gnomeData->myPriority, Signal::REQ_Z };
	sendToAll(packet, 1, gnomeData->gnomes, gnomeData->rank);
	gnomeData->myState = State::WAIT_FOR_PIN;
}

void waitForPin(GnomeData* gnomeData)
{
	gnomeData->acknowledgementCount = gnomeData->pins - gnomeData->ordersCount + 1;
	Packet packet;
	MPI_Status status;

	while (gnomeData->acknowledgementCount < 0)
	{
		MPI_Recv(&packet, sizeof(Packet), MPI_BYTE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);

		if (packet.signal == Signal::REQ_A)
		{
			if (!(packet.priority < gnomeData->myPriority || (packet.priority = gnomeData->myPriority && packet.id < gnomeData->rank)))
			{
				if (!gnomeData->acknowledgementTable[packet.id - 1])
				{
					gnomeData->acknowledgementTable[packet.id - 1] = true;
					gnomeData->acknowledgementCount++;
				}
			}
		}
		else if (packet.signal == Signal::ACK_A)
		{
			if (!gnomeData->acknowledgementTable[packet.id - 1])
			{
				gnomeData->acknowledgementTable[packet.id - 1] = true;
				gnomeData->acknowledgementCount++;
			}
		}
	}
	gnomeData->myState = State::CRITICAL_SECTION_PIN;
}

void criticalSectionPin(GnomeData* gnomeData)
{
	printf("%d taking pin", gnomeData->rank);
	gnomeData->pins -= 1;
	gnomeData->myPriority += 1;
	gnomeData->myState = State::HUNT;
}

void hunt(GnomeData* gnomeData)
{
	printf("%d killing %d hamsters\n", gnomeData->rank, gnomeData->myOrder.hamsters);
	order myOrder = order{ gnomeData->rank, gnomeData->myOrder.hamsters };
	MPI_Send(&myOrder, sizeof(order), MPI_BYTE, 0, 2, MPI_COMM_WORLD);
	gnomeData->myState = State::WAIT;
}

void sendToAll(Packet packet, int tag, int size, int rank)
{
	for (int i = 1; i < size; i++)
	{
		if (i != rank)
		{
			MPI_Send(&packet, sizeof(Packet), MPI_BYTE, i, tag, MPI_COMM_WORLD);
		}
	}
}

void showGnomeData(GnomeData* gnomeData)
{
	printf("%d\n", gnomeData->gnomes);
	printf("%d\n", gnomeData->rank);
	printf("%d\n", gnomeData->myState);
}

void processGnome(GnomeData* gnomeData, bool* syn)
{
	if (gnomeData->myState == State::WAIT)
	{
		wait(gnomeData);
	}
	if (gnomeData->myState == State::GET_ORDER)
	{
		getOrder(gnomeData);
	}
	if (gnomeData->myState == State::WAIT_FOR_ORDER)
	{
		wairForOrder(gnomeData, syn);
	}
	if (gnomeData->myState == State::HUNT)
	{
		hunt(gnomeData);
	}
	if (gnomeData->myState == State::WAIT)
	{
		processGnome(gnomeData, syn);
	}
	/*switch (gnomeData->myState)
	{
	case State::WAIT:
	{
		wait(gnomeData);
	}
	case State::GET_ORDER:
	{
		getOrder(gnomeData);
	}
	case State::WAIT_FOR_ORDER:
	{
		wairForOrder(gnomeData, syn);
	}
	case State::GET_PIN:
	{
		getPin(gnomeData);
	}
	case State::WAIT_FOR_PIN:
	{
		waitForPin(gnomeData);
	}
	case State::CRITICAL_SECTION_PIN:
	{
		criticalSectionPin(gnomeData);
	}
	case State::HUNT:
	{
		hunt(gnomeData);
		break;
	}
	}*/
}

void gnomeMonitor(GnomeData* gnomeData)
{
	Packet packet;
	MPI_Status status;
	Gnome* me;
	Gnome* other;
	me = new Gnome{ gnomeData->rank, gnomeData->myPriority };
	MPI_Recv(&packet, sizeof(Packet), MPI_BYTE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);

	other = new Gnome{ packet.id, packet.priority };

	if (signalState[packet.signal] == gnomeData->myState)
	{
		if (packet.signal == Signal::ACK_A || packet.signal == Signal::ACK_T || packet.signal == Signal::ACK_Z)
		{
			if (gnomeData->acknowledgementTable[packet.id] == false)
			{
				gnomeData->acknowledgementTable[packet.id] = true;
				gnomeData->acknowledgementCount += 1;
			}
			gnomeData->receivedSignals += 1;
		}
		else
		{
			if (!compareGnomes(me, other))
			{
				Packet response = Packet{ gnomeData->rank, gnomeData->myPriority, defaultResponse[packet.signal] };
				MPI_Send(&response, sizeof(Packet), MPI_BYTE, packet.id, 1, MPI_COMM_WORLD);
				gnomeData->receivedSignals += 1;
			}
		}
		
	}
	else
	{ 
		if (!(packet.signal == Signal::ACK_A || packet.signal == Signal::ACK_T || packet.signal == Signal::ACK_Z))
		{
			Packet response = Packet{ gnomeData->rank, gnomeData->myPriority, defaultResponse[packet.signal] };
			MPI_Send(&response, sizeof(Packet), MPI_BYTE, packet.id, 1, MPI_COMM_WORLD);
		}
		else
		{
			gnomeData->acknowledgementCount += 1;
		}
		gnomeData->receivedSignals += 1;
	}
	delete other;
	delete me;
}
