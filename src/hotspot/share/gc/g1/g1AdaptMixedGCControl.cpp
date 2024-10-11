#include "precompiled.hpp"
#include "gc/g1/g1CollectedHeap.inline.hpp"
#include "gc/g1/g1AdaptMixedGCControl.hpp"
#include "gc/g1/g1Predictions.hpp"
#include "gc/g1/g1Trace.hpp"
#include "logging/log.hpp"

G1AdaptMixedGCControl::G1AdaptMixedGCControl(uintx heap_waste_percent,
                                           uintx mixed_gc_live_threshold_percent,
                                           uintx old_cset_region_threshold_percent,
                                           uintx mixed_gc_count_target,
                                           bool adapt_heap_waste_percent,
                                           bool adapt_mixed_gc_live_threshold_percent,
                                           bool adapt_old_cset_region_threshold_percent,
                                           bool adapt_mixed_gc_count_target,
                                           const G1Predictions * predictor,
                                           const G1OldGenAllocationTracker* old_gen_alloc_tracker) :
  _heap_waste_percent(heap_waste_percent),
  _mixed_gc_live_threshold_percent(mixed_gc_live_threshold_percent),
  _old_cset_region_threshold_percent(old_cset_region_threshold_percent),
  _mixed_gc_count_target(mixed_gc_count_target),
  _adapt_heap_waste_percent(adapt_heap_waste_percent),
  _adapt_mixed_gc_live_threshold_percent(adapt_mixed_gc_live_threshold_percent),
  _adapt_old_cset_region_threshold_percent(adapt_old_cset_region_threshold_percent),
  _adapt_mixed_gc_count_target(adapt_mixed_gc_count_target),
  _predictor(predictor),
  _old_gen_alloc_tracker(old_gen_alloc_tracker),
  _last_used_after_gc(-1)
{   
    assert(_heap_waste_percent > 0 && _heap_waste_percent <= 100, "Heap waste percent must be between 0 and 100 but is %zu", _heap_waste_percent);
    assert(_mixed_gc_live_threshold_percent > 0 && _mixed_gc_live_threshold_percent <= 100, "Mixed GC live threshold percent must be between 0 and 100 but is %zu", _mixed_gc_live_threshold_percent);
    assert(_old_cset_region_threshold_percent > 0 && _old_cset_region_threshold_percent <= 100, "Old CSet region threshold percent must be between 0 and 100 but is %zu", _old_cset_region_threshold_percent);
    assert(_mixed_gc_count_target > 0, "Mixed GC count target must be > 0 but is %zu", _mixed_gc_count_target);
    _default_heap_waste_percent_upper_bound = MIN2((uintx)100,_heap_waste_percent*150/100);
    _default_mixed_gc_live_threshold_percent_upper_bound = MIN2((uintx)100,_mixed_gc_live_threshold_percent*110/100);
    _default_old_cset_region_threshold_percent_upper_bound = MIN2((uintx)100,_old_cset_region_threshold_percent*175/100);
    _default_mixed_gc_count_target_upper_bound = MIN2((uintx)100,_mixed_gc_count_target*150/100);

    _default_heap_waste_percent_lower_bound = MAX2((uintx)0,_heap_waste_percent*50/100);
    _default_mixed_gc_live_threshold_percent_lower_bound = MAX2((uintx)0,_mixed_gc_live_threshold_percent*90/100);
    _default_old_cset_region_threshold_percent_lower_bound = MAX2((uintx)0,_old_cset_region_threshold_percent*25/100);
    _default_mixed_gc_count_target_lower_bound = MAX2((uintx)0,_mixed_gc_count_target*50/100);
    _distance_to_target = 0;
}
void G1AdaptMixedGCControl::update_last_young_pause_info(long used_after_gc,long ihop_target) {
    _distance_to_target = used_after_gc - ihop_target;
    _last_used_after_gc = used_after_gc; 
}
void G1AdaptMixedGCControl::update_allocation_info(long used_after_gc) {
   
    long used_since_last_gc = used_after_gc - _last_used_after_gc;
    log_debug(gc,adapt)("last used after gc: %ld, used after last gc: %ld, used since last gc: %ld", _last_used_after_gc, used_after_gc, used_since_last_gc);
    _old_gen_growth_s.add((double)used_since_last_gc);
    _last_used_after_gc = used_after_gc;
    if(have_enough_data_for_prediction()) {
        double pred_old_gen_growth = predict(&_old_gen_growth_s);
        
        log_debug(gc,adapt)("predict old gen growth: %f, _distance_to_target/_mixed_gc_count_target: %f", pred_old_gen_growth, (double)(_distance_to_target/_mixed_gc_count_target));
        if((-pred_old_gen_growth) < (double)(_distance_to_target/_mixed_gc_count_target)*0.8) {
            if(_adapt_mixed_gc_count_target) {
            _mixed_gc_count_target = MAX2(_default_mixed_gc_count_target_lower_bound,_mixed_gc_count_target * 95/100);
            }
            if(_adapt_old_cset_region_threshold_percent) {
                _old_cset_region_threshold_percent = MIN2(_default_old_cset_region_threshold_percent_upper_bound,(_old_cset_region_threshold_percent * 105+99)/100);
            }
            if(_adapt_mixed_gc_live_threshold_percent) {
                _mixed_gc_live_threshold_percent = MIN2(_default_mixed_gc_live_threshold_percent_upper_bound,(_mixed_gc_live_threshold_percent * 105+99)/100);
            }
            
            if(_adapt_heap_waste_percent) {
                _heap_waste_percent = MAX2(_default_heap_waste_percent_lower_bound,_heap_waste_percent * 95/100);
            }
        }else if ((-pred_old_gen_growth) > (double)(_distance_to_target/_mixed_gc_count_target)*1.2) {
            if(_adapt_mixed_gc_count_target) {
            _mixed_gc_count_target = MIN2(_default_mixed_gc_count_target_upper_bound,(_mixed_gc_count_target * 105+99)/100);
            }
            if(_adapt_old_cset_region_threshold_percent) {
            _old_cset_region_threshold_percent = MAX2(_default_old_cset_region_threshold_percent_lower_bound,(_old_cset_region_threshold_percent * 95)/100);
            }
            if(_adapt_mixed_gc_live_threshold_percent) {
            _mixed_gc_live_threshold_percent = MAX2(_default_mixed_gc_live_threshold_percent_lower_bound,(_mixed_gc_live_threshold_percent * 95)/100);
            }
            if(_adapt_heap_waste_percent) {
            _heap_waste_percent = MIN2(_default_heap_waste_percent_upper_bound,(_heap_waste_percent * 105+99)/100);
            }
        }
            
        
        print();
    }
}

double G1AdaptMixedGCControl::predict(TruncatedSeq const* seq) const {
        return _predictor->predict(seq);
}

bool G1AdaptMixedGCControl::have_enough_data_for_prediction() const {
    return ((size_t)_old_gen_growth_s.num() >= G1AdaptMixedGCNumInitialSamples);
}

void G1AdaptMixedGCControl::print() {
    log_debug(gc,adapt)("Adaptive mixed GC information (value update), current values"
                               "heap waste percent:" UINTX_FORMAT ", mixed GC live threshold percent:" UINTX_FORMAT ", old CSet region threshold percent: " UINTX_FORMAT ", mixed GC count target:" UINTX_FORMAT ", "
                               "prediction active: %s",
                               _heap_waste_percent, _mixed_gc_live_threshold_percent, _old_cset_region_threshold_percent, _mixed_gc_count_target,
                               have_enough_data_for_prediction() ? "true" : "false");
}
