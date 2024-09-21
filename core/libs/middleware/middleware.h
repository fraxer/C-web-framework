#ifndef __MIDDLEWARE__
#define __MIDDLEWARE__

#include "macros.h"

typedef int (*middleware_fn_p)(void*);

typedef struct middleware_item {
    middleware_fn_p fn;
    struct middleware_item* next;
} middleware_item_t;

typedef struct middleware_global_fn {
    char name[128];
    middleware_fn_p fn;
} middleware_global_fn_t;

#define MW_VA_NARGS_REVERSE_(_63,_62,_61,_60,_59,_58,_57,_56,_55,_54,_53,_52,_51,_50,_49,_48,_47,_46,_45,_44,_43,_42,_41,_40,_39,_38,_37,_36,_35,_34,_33,_32,_31,_30,_29,_28,_27,_26,_25,_24,_23,_22,_21,_20,_19,_18,_17,_16,_15,_14,_13,_12,_11,_10,_9,_8,_7,_6,_5,_4,_3,_2,_1,N,...) N
#define MW_VA_NARGS(...) MW_VA_NARGS_REVERSE_(__VA_ARGS__, 63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define MW_ITEM_CONCAT(A, B) A##B
#define MW_ITEM(N, ...) MW_ITEM_CONCAT(MW_ITEM_, N)(__VA_ARGS__)
#define MW_ITEM_1(NAME) NAME
#define MW_ITEM_2(NAME, ...)  NAME && MW_ITEM_1(__VA_ARGS__)
#define MW_ITEM_3(NAME, ...)  NAME && MW_ITEM_2(__VA_ARGS__)
#define MW_ITEM_4(NAME, ...)  NAME && MW_ITEM_3(__VA_ARGS__)
#define MW_ITEM_5(NAME, ...)  NAME && MW_ITEM_4(__VA_ARGS__)
#define MW_ITEM_6(NAME, ...)  NAME && MW_ITEM_5(__VA_ARGS__)
#define MW_ITEM_7(NAME, ...)  NAME && MW_ITEM_6(__VA_ARGS__)
#define MW_ITEM_8(NAME, ...)  NAME && MW_ITEM_7(__VA_ARGS__)
#define MW_ITEM_9(NAME, ...)  NAME && MW_ITEM_8(__VA_ARGS__)
#define MW_ITEM_10(NAME, ...) NAME && MW_ITEM_9(__VA_ARGS__)
#define MW_ITEM_11(NAME, ...) NAME && MW_ITEM_10(__VA_ARGS__)
#define MW_ITEM_12(NAME, ...) NAME && MW_ITEM_11(__VA_ARGS__)
#define MW_ITEM_13(NAME, ...) NAME && MW_ITEM_12(__VA_ARGS__)
#define MW_ITEM_14(NAME, ...) NAME && MW_ITEM_13(__VA_ARGS__)
#define MW_ITEM_15(NAME, ...) NAME && MW_ITEM_14(__VA_ARGS__)
#define MW_ITEM_16(NAME, ...) NAME && MW_ITEM_15(__VA_ARGS__)
#define MW_ITEM_17(NAME, ...) NAME && MW_ITEM_16(__VA_ARGS__)
#define MW_ITEM_18(NAME, ...) NAME && MW_ITEM_17(__VA_ARGS__)
#define MW_ITEM_19(NAME, ...) NAME && MW_ITEM_18(__VA_ARGS__)
#define MW_ITEM_20(NAME, ...) NAME && MW_ITEM_19(__VA_ARGS__)
#define MW_ITEM_21(NAME, ...) NAME && MW_ITEM_20(__VA_ARGS__)
#define MW_ITEM_22(NAME, ...) NAME && MW_ITEM_21(__VA_ARGS__)
#define MW_ITEM_23(NAME, ...) NAME && MW_ITEM_22(__VA_ARGS__)
#define MW_ITEM_24(NAME, ...) NAME && MW_ITEM_23(__VA_ARGS__)
#define MW_ITEM_25(NAME, ...) NAME && MW_ITEM_24(__VA_ARGS__)
#define MW_ITEM_26(NAME, ...) NAME && MW_ITEM_25(__VA_ARGS__)
#define MW_ITEM_27(NAME, ...) NAME && MW_ITEM_26(__VA_ARGS__)
#define MW_ITEM_28(NAME, ...) NAME && MW_ITEM_27(__VA_ARGS__)
#define MW_ITEM_29(NAME, ...) NAME && MW_ITEM_28(__VA_ARGS__)
#define MW_ITEM_30(NAME, ...) NAME && MW_ITEM_29(__VA_ARGS__)
#define MW_ITEM_31(NAME, ...) NAME && MW_ITEM_30(__VA_ARGS__)
#define MW_ITEM_32(NAME, ...) NAME && MW_ITEM_31(__VA_ARGS__)
#define MW_ITEM_33(NAME, ...) NAME && MW_ITEM_32(__VA_ARGS__)
#define MW_ITEM_34(NAME, ...) NAME && MW_ITEM_33(__VA_ARGS__)
#define MW_ITEM_35(NAME, ...) NAME && MW_ITEM_34(__VA_ARGS__)
#define MW_ITEM_36(NAME, ...) NAME && MW_ITEM_35(__VA_ARGS__)
#define MW_ITEM_37(NAME, ...) NAME && MW_ITEM_36(__VA_ARGS__)
#define MW_ITEM_38(NAME, ...) NAME && MW_ITEM_37(__VA_ARGS__)
#define MW_ITEM_39(NAME, ...) NAME && MW_ITEM_38(__VA_ARGS__)
#define MW_ITEM_40(NAME, ...) NAME && MW_ITEM_39(__VA_ARGS__)
#define MW_ITEM_41(NAME, ...) NAME && MW_ITEM_40(__VA_ARGS__)
#define MW_ITEM_42(NAME, ...) NAME && MW_ITEM_41(__VA_ARGS__)
#define MW_ITEM_43(NAME, ...) NAME && MW_ITEM_42(__VA_ARGS__)
#define MW_ITEM_44(NAME, ...) NAME && MW_ITEM_43(__VA_ARGS__)
#define MW_ITEM_45(NAME, ...) NAME && MW_ITEM_44(__VA_ARGS__)
#define MW_ITEM_46(NAME, ...) NAME && MW_ITEM_45(__VA_ARGS__)
#define MW_ITEM_47(NAME, ...) NAME && MW_ITEM_46(__VA_ARGS__)
#define MW_ITEM_48(NAME, ...) NAME && MW_ITEM_47(__VA_ARGS__)
#define MW_ITEM_49(NAME, ...) NAME && MW_ITEM_48(__VA_ARGS__)
#define MW_ITEM_50(NAME, ...) NAME && MW_ITEM_49(__VA_ARGS__)
#define MW_ITEM_51(NAME, ...) NAME && MW_ITEM_50(__VA_ARGS__)
#define MW_ITEM_52(NAME, ...) NAME && MW_ITEM_51(__VA_ARGS__)
#define MW_ITEM_53(NAME, ...) NAME && MW_ITEM_52(__VA_ARGS__)
#define MW_ITEM_54(NAME, ...) NAME && MW_ITEM_53(__VA_ARGS__)
#define MW_ITEM_55(NAME, ...) NAME && MW_ITEM_54(__VA_ARGS__)
#define MW_ITEM_56(NAME, ...) NAME && MW_ITEM_55(__VA_ARGS__)
#define MW_ITEM_57(NAME, ...) NAME && MW_ITEM_56(__VA_ARGS__)
#define MW_ITEM_58(NAME, ...) NAME && MW_ITEM_57(__VA_ARGS__)
#define MW_ITEM_59(NAME, ...) NAME && MW_ITEM_58(__VA_ARGS__)
#define MW_ITEM_60(NAME, ...) NAME && MW_ITEM_59(__VA_ARGS__)
#define MW_ITEM_61(NAME, ...) NAME && MW_ITEM_60(__VA_ARGS__)
#define MW_ITEM_62(NAME, ...) NAME && MW_ITEM_61(__VA_ARGS__)
#define MW_ITEM_63(NAME, ...) NAME && MW_ITEM_62(__VA_ARGS__)

#define middleware(...) if (!(MW_ITEM(MW_VA_NARGS(__VA_ARGS__), __VA_ARGS__))) return;

int run_middlewares(middleware_item_t* middleware_item, void* ctx);
middleware_item_t* middleware_create(middleware_fn_p fn);
void middlewares_free(middleware_item_t* middleware_item);

#endif