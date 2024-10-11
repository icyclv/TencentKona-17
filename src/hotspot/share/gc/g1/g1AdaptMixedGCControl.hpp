#ifndef SHARE_GC_G1_G1ADAPTMIXEDGCCONTROL_HPP
#define SHARE_GC_G1_G1ADAPTMIXEDGCCONTROL_HPP


#include "gc/g1/g1OldGenAllocationTracker.hpp"
#include "memory/allocation.hpp"
#include "utilities/numberSeq.hpp"

class G1Predictions;
class G1NewTracer;

class G1AdaptMixedGCControl : public CHeapObj<mtGC> {
    uintx _heap_waste_percent;
    uintx _mixed_gc_live_threshold_percent;
    uintx _old_cset_region_threshold_percent;
    uintx _mixed_gc_count_target;
    bool _adapt_heap_waste_percent;
    bool _adapt_mixed_gc_live_threshold_percent;
    bool _adapt_old_cset_region_threshold_percent;
    bool _adapt_mixed_gc_count_target;
    const G1Predictions * _predictor;

    const G1OldGenAllocationTracker* _old_gen_alloc_tracker;


    TruncatedSeq _old_gen_growth_s;
    uintx _default_heap_waste_percent_upper_bound;
    uintx _default_mixed_gc_live_threshold_percent_upper_bound;
    uintx _default_old_cset_region_threshold_percent_upper_bound;
    uintx _default_mixed_gc_count_target_upper_bound;
    uintx _default_heap_waste_percent_lower_bound;
    uintx _default_mixed_gc_live_threshold_percent_lower_bound;
    uintx _default_old_cset_region_threshold_percent_lower_bound;
    uintx _default_mixed_gc_count_target_lower_bound;
    long _last_used_after_gc;

    double predict(TruncatedSeq const* seq) const;

    bool have_enough_data_for_prediction() const;

 protected:
    virtual double last_old_gen_growth() const { return _old_gen_growth_s.last(); }

 public:
    G1AdaptMixedGCControl(uintx heap_waste_percent,
                          uintx mixed_gc_live_threshold_percent,
                          uintx old_cset_region_threshold_percent,
                          uintx mixed_gc_count_target,
                          bool adapt_heap_waste_percent,
                          bool adapt_mixed_gc_live_threshold_percent,
                          bool adapt_old_cset_region_threshold_percent,
                          bool adapt_mixed_gc_count_target,
                          const G1Predictions * predictor,
                          const G1OldGenAllocationTracker* old_gen_alloc_tracker);

    virtual void update_allocation_info(long used_after_gc,bool is_last_young_pause);

    virtual size_t get_heap_waste_percent() const { return _heap_waste_percent; }
    virtual size_t get_mixed_gc_live_threshold_percent() const { return _mixed_gc_live_threshold_percent; }
    virtual size_t get_old_cset_region_threshold_percent() const { return _old_cset_region_threshold_percent; }
    virtual size_t get_mixed_gc_count_target() const { return _mixed_gc_count_target; }

    virtual void print();





};
#endif