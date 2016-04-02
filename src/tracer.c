#include "tracer.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

//================================================================
//----------------------------------------------------------------
//================================================================

static inline double mrand() {
	return ((double)rand() / RAND_MAX);
}

static inline double rrand(double min, double max) {
	return mrand() * (max - min) + min;
}

static inline int padj(void * t) {
	if (((tracer_common_t *)(t))->period != ((tracer_common_t *)(t))->period_cache) {
		((tracer_common_t *)(t))->period_cache = ((tracer_common_t *)(t))->period;
		((tracer_common_t *)(t))->period_adj = ((tracer_common_t *)(t))->period / *((tracer_common_t *)(t))->sample_rate_ptr;
		return 1;
	}
	return 0;
}

extern inline void tracer_common_init(tracer_common_t * tc, double period, sample_rate_t * sample_rate_ptr);
extern inline double tracer_common_next(void * t);
extern inline void tracer_mod_init(tracer_mod_t * tm, void * affector, double * target_data, double min, double max);
extern inline void tracer_mod_affect(tracer_mod_t * tm);

//================================================================

tracer_sine_t tracer_sine_create(double period, sample_rate_t * sample_rate_ptr) {
	tracer_sine_t t;
	tracer_common_init(&t.common, period, sample_rate_ptr);
	t.common.next = (tracer_next_f)tracer_sine_next;
	t.theta = -M_PI/2;
	return t;
}

double tracer_sine_next(tracer_sine_t * ts) {
	padj(ts);
	ts->theta += ts->common.period_adj * M_PI * 2;
	while (ts->theta >= M_PI * 2) {
		ts->theta -= M_PI * 2;
	}
	return sin(ts->theta);
}

//================================================================

tracer_saw_t tracer_saw_create(double period, sample_rate_t * sample_rate_ptr) {
	tracer_saw_t t;
	tracer_common_init(&t.common, period, sample_rate_ptr);
	t.common.next = (tracer_next_f)tracer_saw_next;
	t.lv = -1;
	return t;
}

double tracer_saw_next(tracer_saw_t * ts) {
	padj(ts);
	ts->lv += ts->common.period_adj;
	while (ts->lv >= 1) {
		ts->lv -= 2;
	}
	return ts->lv;
}

//================================================================

tracer_freqnoise_t tracer_freqnoise_create(double period, sample_rate_t * sample_rate_ptr, uint_fast8_t subtracer_count, double subtracer_freqmod, double freqdev) {
	tracer_freqnoise_t t;
	tracer_common_init(&t.common, period, sample_rate_ptr);
	t.common.next = (tracer_next_f)tracer_freqnoise_next;
	t.subtracers_len = subtracer_count;
	t.subtracers = (tracer_freqnoise_subtracer_t *)malloc(subtracer_count * sizeof(tracer_freqnoise_subtracer_t));
	t.freqmod = subtracer_freqmod;
	t.freqdev = freqdev;
	tracer_freqnoise_subtracer_t * ts = t.subtracers;
	for (int i = 0; i < subtracer_count; i++, ts++) {
		ts->period_adj_sub = t.common.period_adj + (((i/(double)subtracer_count)-0.5) * subtracer_freqmod * t.common.period_adj);
		ts->mult = 1;
		ts->theta = 0;
		ts->direc = 1;
		ts->tmult = 1;
	}
	return t;
}

void tracer_freqnoise_destroy(tracer_freqnoise_t * t) {
	free(t->subtracers);
}

#define TEMP 5

double tracer_freqnoise_next(tracer_freqnoise_t * t) {
	tracer_freqnoise_subtracer_t * ts = t->subtracers;
	if (padj(t)) {
		for (int i = 0; i < t->subtracers_len; i++, ts++) {
			ts->period_adj_sub = t->common.period_adj + (((i/(double)t->subtracers_len)-0.5) * t->freqmod * t->common.period_adj);
		}
		ts = t->subtracers;
	}
	double ret = 0;
	for (int i = 0; i < t->subtracers_len; i++, ts++) {
		switch(ts->direc) {
		case 1:
			ts->theta += ts->period_adj_sub * 2 * ts->tmult * M_PI;
			goto direcU;
		case -1:
			ts->theta -= ts->period_adj_sub * 2 * ts->tmult * M_PI;
			goto direcD;
		}
		direcU:
		if (ts->theta > 3 * M_PI / 2) {
			ts->theta = 2*(3 * M_PI / 2) - ts->theta;
			
			ts->tmult += rrand(t->freqdev/TEMP, -t->freqdev/TEMP);
			if (ts->tmult > 1+t->freqdev) ts->tmult = 1+t->freqdev;
			if (ts->tmult < 1-t->freqdev) ts->tmult = 1-t->freqdev;
			
			ts->direc = -1;
			goto direcD;
		}
		ret += (0.5+0.5*sin(ts->theta)) * 2 - 1;
		continue;
		
		direcD:
		if (ts->theta < M_PI / 2) {
			ts->theta = 2*(M_PI / 2) - ts->theta;
			
			ts->tmult += rrand(t->freqdev/TEMP, -t->freqdev/TEMP);
			if (ts->tmult > t->freqdev) ts->tmult = t->freqdev;
			if (ts->tmult < -t->freqdev) ts->tmult = -t->freqdev;
			
			ts->direc = 1;
			goto direcU;
		}
		ret += (0.5+0.5*sin(ts->theta)) * 2 - 1;
		continue;
	}
	return ret/t->subtracers_len;
}

//================================================================

/*

void tracer_sine_init( struct tracer_sine_s * restrict tt , double period , uint_fast32_t sample_rate ) {
	tt->period = period;
	tt->sample_rate = sample_rate;
	tt->period_adj = period / sample_rate;
	tt->theta = 0;
}

extern inline double tracer_sine_next_sine( struct tracer_sine_s * restrict tt );
extern inline double tracer_sine_next_cosine( struct tracer_sine_s * restrict tt );

//================================================================

void tracer_sine_epo_init( struct tracer_sine_epo_s * restrict tt , double period , uint_fast32_t sample_rate ) {
	tt->period = period;
	tt->sample_rate = sample_rate;
	tt->period_adj = period / sample_rate;
	tt->theta = 0;
	tt->p = 0;
}

//================================================================
//----------------------------------------------------------------
//================================================================


//============================= TRIG =============================

void tracer_trig_init(tracer_trig_t * tt, double period, unsigned int rate) {
	tt->period = period;
	TRACER_RATE(tt, rate);
	tt->theta = 0;
}

double tracer_trig_next_sin(tracer_trig_t * tt) {
	tt->theta += tt->period_adj * M_PI * 2;
	while (tt->theta >= M_PI * 2) {
		tt->theta -= M_PI * 2;
	}
	return sin(tt->theta);
}

double tracer_trig_next_cos(tracer_trig_t * tt) {
	tt->theta += tt->period_adj * M_PI * 2;
	while (tt->theta >= M_PI * 2) {
		tt->theta -= M_PI * 2;
	}
	return cos(tt->theta);
}

double tracer_trig_next_tan(tracer_trig_t * tt) {
	tt->theta += tt->period_adj * M_PI * 2;
	while (tt->theta >= M_PI * 2) {
		tt->theta -= M_PI * 2;
	}
	return tan(tt->theta);
}

//=========================== TRIANGLE ===========================

void tracer_triangle_init (tracer_triangle_t * tt, double period, unsigned int rate) {
	tt->period = period;
	TRACER_RATE(tt, rate);
	tt->lv = 0;
}

double tracer_triangle_next (tracer_triangle_t * tt) {
	tt->lv -= tt->period_adj * 2;
	while(tt->lv < -1.0) {tt->lv += 2;}
	return fabs(tt->lv) * 2 - 1;
}

//============================ SQUARE ============================

void tracer_square_init(tracer_square_t * ts, double period, unsigned int rate) {
	ts->period = period;
	TRACER_RATE(ts, rate);
	ts->lv = -1;
}

double tracer_square_next(tracer_square_t * ts) {
	ts->lv += ts->period_adj * 2;
	while (ts->lv > 1) {ts->lv -= 2;}
	if (ts->lv > 0) return 1;
	else return -1;
}

//============================ GNOISE ============================

void tracer_gnoise_init(tracer_gnoise_t * tg) {
	tg->period = 0;
	tg->period_adj = 0;
	tg->lv = 0;
	tg->acc = 0;
}

double tracer_gnoise_next(tracer_gnoise_t * tg) {
	tg->acc = -tg->acc;
	tg->lv += tg->acc;
	if (tg->lv > 1) tg->lv = 1;
	if (tg->lv < -1) tg->lv = -1;
	return tg->lv;
}

//=========================== DUMBFIRE ===========================

void tracer_dumbfire_init(tracer_dumbfire_t * tg) {
	tg->period = 0;
	tg->period_adj = 0;
	tg->lv = 0;
}

double tracer_dumbfire_next(tracer_dumbfire_t * tg) {
	tg->acc += 5*(2*(((double)rand()) / ((double)RAND_MAX) - 0.5));
	if (tg->acc > 0.02) tg->acc = 0.02;
	if (tg->acc < -0.02) tg->acc = -0.02;
	tg->lv += tg->acc;
	if (tg->lv > 1) tg->lv = 1;
	if (tg->lv < -1) tg->lv = -1;
	return tg->lv;
}
*/
