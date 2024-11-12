#define CECS_IMPLEMENTATION
#include "cecs.h"

typedef struct {
	int x, y;
} Transform;

int main() {
	CECS cecs = cecs_new(10, 10);

	Entity ent_1 = entity_new(&cecs);
	entity_add_component(&cecs, ent_1, Transform, {
		.x = 10,
		.y = 10
	});
	printf("ent_1: %d\n", ent_1);

	Entity ent_2 = entity_new(&cecs);
	entity_add_component(&cecs, ent_2, Transform, {
		.x = 20,
		.y = 20
	});
	printf("ent_2: %d\n", ent_2);

	printf("Before changing\n");
	cecs_for_each(&cecs, Transform, CECS_LAMBDA(
		void, (Entity id, Transform* tc) {
			printf("ent: %d, data: %d %d\n", id, tc->x, tc->y);
			tc->x += 10;
			tc->y += 10;
		}
	));

	printf("After changing\n");
	cecs_for_each(&cecs, Transform, CECS_LAMBDA(
		void, (Entity id, Transform* tc) {
			printf("ent: %d, data: %d %d\n", id, tc->x, tc->y);
		}
	));

	return 0;
}
