#include "core/simple-timer.h"
#include "core/system.h"

void simple_timer_setup(struct simple_timer *t, uint64_t wait_time, bool auto_reset)
{
	t->wait_time   = wait_time;
	t->auto_reset  = auto_reset;
	t->target_time = system_get_ticks() + wait_time;
}

bool simple_timer_has_elapsed(struct simple_timer *t)
{
	if (t->has_elapsed_before && !t->auto_reset) {
		return false;
	}

	uint64_t now	     = system_get_ticks();
	bool	 has_elapsed = now >= t->target_time;

	if (has_elapsed) {
		t->has_elapsed_before = true;
		if (t->auto_reset) {
			t->target_time = t->wait_time + t->target_time;
		}
	}

	return has_elapsed;
}

void simple_timer_reset(struct simple_timer *t)
{
	simple_timer_setup(t, t->wait_time, t->auto_reset);
}