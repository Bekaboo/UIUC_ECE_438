#ifndef __ALG_H__
#define __ALG_H__

#define min(a,b) ({                                       \
    typeof(a) _a_temp_;                                   \
    typeof(b) _b_temp_;                                   \
    _a_temp_ = (a);                                       \
    _b_temp_ = (b);                                       \
    _a_temp_ = _a_temp_ < _b_temp_ ? _a_temp_ : _b_temp_; \
})

#define max(a,b) ({                                       \
    typeof(a) _a_temp_;                                   \
    typeof(b) _b_temp_;                                   \
    _a_temp_ = (a);                                       \
    _b_temp_ = (b);                                       \
    _a_temp_ = _a_temp_ < _b_temp_ ? _b_temp_ : _a_temp_; \
})

#endif /* __ALG_H__ */
