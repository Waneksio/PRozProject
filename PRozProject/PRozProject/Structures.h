#pragma once
#include <vector>

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

struct GnomeData
{
	int rank;
	int pins;
	int poisons;
	int ordersCount;
	int gnomes;
	int acknowledgementCount;
	int receivedSignals;
	State myState;
	order myOrder;
	int myPriority;
	bool* acknowledgementTable;
	int* hamstersToKill;
	std::vector<Gnome*> gnomesOrder;
	order* orders;
};