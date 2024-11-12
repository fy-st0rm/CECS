#ifndef __CECS_H__
#define __CECS_H__

#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef alloc
#define alloc malloc
#endif

#ifndef clean
#define clean free
#endif

#undef assert
#define assert(x, ...)\
	do {\
		if (!(x)) {\
			fprintf(stderr, "\033[31m[ASSERTION]: %s:%d:\033[0m ", __FILE__, __LINE__);\
			fprintf(stderr, __VA_ARGS__);\
			exit(1);\
		}\
	} while(0);\

#ifndef rand_init
#define rand_init(seed) srand(seed)
#endif

#ifndef rand_range
#define rand_range(l, u) rand() % (u - l) + l
#endif

#define CECS_LAMBDA(return_type, body) \
	({ return_type __fn__ body __fn__; })

int cecs_hash_data(char* data);

/*
 * Entity type
 */

typedef unsigned int Entity;

typedef enum {
	FREE, OCCUPIED
} SlotState;

/*
 * A record that stores the component data
 */

typedef struct {
	void** data;
	int x;
} CompRecord;

/*
 * Entity slot that holds entity state and
 * all its components
 */

typedef struct {
	Entity id;
	SlotState state;
	SlotState* components;
} EntitySlot;

/*
 * Entity Component System
 */

typedef struct {
	int entity_cnt;
	int max_entities;
	int max_components;
	EntitySlot* entities;
	CompRecord** components;
} CECS;

CECS cecs_new(int max_entities, int max_components);
void cecs_delete(CECS* cecs);
Entity entity_new(CECS* cecs);
int entity_delete(CECS* cecs, Entity id);

void* __entity_add_component(CECS* cecs, Entity entity, char* name, void* data);
#define entity_add_component(cecs, entity, component, ...) ({\
	component c = __VA_ARGS__;                                 \
	component* data = alloc(sizeof(c));                        \
	memcpy(data, &c, sizeof(c));                               \
	__entity_add_component(cecs, entity, #component, data);    \
})

void* __entity_get_component(CECS* cecs, Entity entity, char* name);
#define entity_get_component(cecs, entity, component)\
	__entity_get_component(cecs, entity, #component)

int __entity_remove_component(CECS* cecs, Entity entity, char* name);
#define entity_remove_component(cecs, entity, component)\
	__entity_remove_component(cecs, entity, #component)

#define cecs_for_each(cecs, component, lamda) ({\
	int hash = cecs_hash_data(#component);\
	int idx = hash % (cecs)->max_components;\
	CompRecord* record = (cecs)->components[idx];\
	for (int __i = 0; __i < (cecs)->max_entities; __i++) {\
		void* data = record->data[__i];\
		if (data) {\
			lamda(__i, data);\
		}\
	}\
})


/*
 * Implementations
 */

#ifdef CECS_IMPLEMENTATION

int cecs_hash_data(char* data) {
	int hash = 1000;
	int c;
	while ((c = *data++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

CECS cecs_new(int max_entities, int max_components) {
	rand_init(time(NULL));

	EntitySlot* entities = alloc(sizeof(EntitySlot) * max_entities);
	for (int i = 0; i < max_entities; i++) {
		int size = sizeof(SlotState) * max_components;
		entities[i].components = alloc(size);
		memset(entities[i].components, 0, size);
	}

	CompRecord** components = alloc(sizeof(CompRecord*) * max_components);
	for (int i = 0; i < max_components; i++) {
		CompRecord* record = alloc(sizeof(CompRecord));

		int size = sizeof(void*) * max_entities;
		record->data = alloc(size);
		memset(record->data, 0, size);

		components[i] = record;
	}

	return (CECS) {
		.entity_cnt = 0,
		.max_entities = max_entities,
		.max_components = max_components,
		.entities = entities,
		.components = components
	};
}

void cecs_delete(CECS* cecs) {
	for (int i = 0; i < cecs->max_entities; i++) {
		clean(cecs->entities[i].components);
	}

	for (int i = 0; i < cecs->max_components; i++) {
		CompRecord* record = cecs->components[i];
		for (int j = 0; j < cecs->max_entities; j++) {
			void* data = record->data[j];
			if (data) {
				clean(data);
			}
		}
		clean(record);
	}

	clean(cecs->entities);
	clean(cecs->components);
}

Entity entity_new(CECS* cecs) {
	assert(
		cecs->entity_cnt < cecs->max_entities,
		"Entity slots are full.\n"
	);
	cecs->entity_cnt++;

	// Generating random entity id
	Entity id = rand_range(0, cecs->max_entities);
	do {
		id = rand_range(0, cecs->max_entities);
	} while (cecs->entities[id].state != FREE);

	// Making the slot occupied
	cecs->entities[id].state = OCCUPIED;
	return id;
}

int entity_delete(CECS* cecs, Entity id) {
	if (cecs->entities[id].state == FREE) {
		return 0;
	}

	for (int i = 0; i < cecs->max_components; i++) {
		SlotState state = cecs->entities[id].components[i];
		if (state == FREE) continue;
		CompRecord* record = cecs->components[i];
		clean(record->data[id]);
		record->data[id] = NULL;
	}

	cecs->entities[id].state = FREE;
	cecs->entity_cnt--;

	return 1;
}

void* __entity_add_component(CECS* cecs, Entity entity, char* name, void* data) {
	int hash = cecs_hash_data(name);
	int idx = hash % cecs->max_components;

	CompRecord* record = cecs->components[idx];
	record->data[entity] = data;

	// Saving to the entity slot
	cecs->entities[entity].components[idx] = OCCUPIED;
	return data;
}

void* __entity_get_component(CECS* cecs, Entity entity, char* name) {
	int hash = cecs_hash_data(name);
	int idx = hash % cecs->max_components;
	return cecs->components[idx]->data[entity];
}

int __entity_remove_component(CECS* cecs, Entity entity, char* name) {
	int hash = cecs_hash_data(name);
	int idx = hash % cecs->max_components;
	void* data = cecs->components[idx]->data[entity];
	if (data) {
		clean(data);
		cecs->components[idx]->data[entity] = NULL;
		cecs->entities[entity].components[idx] = FREE;
		return 1;
	}
	return 0;
}

#endif // CECS_IMPLEMENTATION

#endif // __CECS_H__
