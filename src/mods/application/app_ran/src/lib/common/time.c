/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "lib/common/time.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libtime"
	
static oset_time_t gnb_current_time = 0;
static int		   gnb_current_time_last_frame = 0;
static int		   gnb_current_time_last_subframe = 0;

typedef struct gnd_timer_mgr_s {
	OSET_POOL(pool, gnb_timer_t);
	oset_rbtree_t tree;
} gnb_timer_mgr_t;

static oset_time_t gnb_get_current_time(void)
{
	return gnb_current_time;
}

static void add_timer_node(
		oset_rbtree_t *tree, gnb_timer_t *timer, oset_time_t duration)
{
	oset_rbnode_t **new = NULL;
	oset_rbnode_t *parent = NULL;
	oset_assert(tree);
	oset_assert(timer);

	timer->timeout = gnb_get_current_time() + duration;

	new = &tree->root;
	while (*new) {
		gnb_timer_t *this = oset_rb_entry(*new, gnb_timer_t, rbnode);

		parent = *new;
		if (timer->timeout < this->timeout)
			new = &(*new)->left;
		else
			new = &(*new)->right;
	}

	oset_rbtree_link_node(timer, parent, new);
	oset_rbtree_insert_color(tree, timer);
}

gnb_timer_mgr_t *gnb_timer_mgr_create(unsigned int capacity)
{
	gnb_timer_mgr_t *manager = oset_calloc(1, sizeof *manager);
	oset_expect_or_return_val(manager, NULL);

	oset_pool_init(&manager->pool, capacity);

	return manager;
}

void gnb_timer_mgr_destroy(gnb_timer_mgr_t *manager)
{
	oset_assert(manager);

	oset_pool_final(&manager->pool);
	oset_free(manager);
}

gnb_timer_t *gnb_timer_add(
		gnb_timer_mgr_t *manager, void (*cb)(void *data), void *data)
{
	gnb_timer_t *timer = NULL;
	oset_assert(manager);

	oset_pool_alloc(&manager->pool, &timer);
	oset_assert(timer);

	memset(timer, 0, sizeof(*timer));
	timer->cb = cb;
	timer->data = data;

	timer->manager = manager;

	return timer;
}

void gnb_timer_delete(gnb_timer_t *timer)
{
	gnb_timer_mgr_t *manager;
	oset_assert(timer);
	manager = timer->manager;
	oset_assert(manager);

	gnb_timer_stop(timer);

	oset_pool_free(&manager->pool, timer);
}

void gnb_timer_start(gnb_timer_t *timer, oset_time_t duration)
{
	gnb_timer_mgr_t *manager = NULL;
	oset_assert(timer);
	oset_assert(duration);

	manager = timer->manager;
	oset_assert(manager);

	if (timer->running == true)
		oset_rbtree_delete(&manager->tree, timer);

	timer->running = true;
	add_timer_node(&manager->tree, timer, duration);
}

void gnb_timer_stop(gnb_timer_t *timer)
{
	gnb_timer_mgr_t *manager = NULL;
	oset_assert(timer);
	manager = timer->manager;
	oset_assert(manager);

	if (timer->running == false)
		return;

	timer->running = false;
	oset_rbtree_delete(&manager->tree, timer);
}

void gnb_timer_mgr_expire(gnb_timer_mgr_t *manager)
{
	OSET_LIST(list);
	oset_lnode_t *lnode;

	oset_time_t current;
	oset_rbnode_t *rbnode;
	gnb_timer_t *this;
	oset_assert(manager);

	current = gnb_get_current_time();

	oset_rbtree_for_each(&manager->tree, rbnode) {
		this = oset_rb_entry(rbnode, gnb_timer_t, rbnode);

		if (this->timeout > current)
			break;

		oset_list_add(&list, &this->lnode);
	}

	oset_list_for_each(&list, lnode) {
		this = oset_rb_entry(lnode, gnb_timer_t, lnode);
		gnb_timer_stop(this);
		if (this->cb)
			this->cb(this->data);
	}
}

void gnb_time_tick(int frame, int subframe)
{
	if (frame != gnb_current_time_last_frame ||
	  subframe != gnb_current_time_last_subframe) {
		gnb_current_time_last_frame = frame;
		gnb_current_time_last_subframe = subframe;
		//1ms add
		gnb_current_time++;
	}
}

