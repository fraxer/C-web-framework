#ifndef __MODEL__
#define __MODEL__

#include <stddef.h>
#include <string.h>
#include <time.h>

#include "database.h"
#include "json.h"
#include "array.h"
#include "str.h"
#include "enums.h"
#include "macros.h"

typedef struct tm tm_t;

tm_t* tm_create(tm_t* time);

#define mnfields(TYPE, FIELD, NAME, VALUE) \
    .type = TYPE,\
    .name = #NAME,\
    .dirty = 0,\
    .value.FIELD = VALUE,\
    .value._string = NULL,\
    .oldvalue._short = 0,\
    .oldvalue._string = NULL

#define mdfields(TYPE, NAME, VALUE) \
    .type = TYPE,\
    .name = #NAME,\
    .dirty = 0,\
    .value._tm = tm_create(VALUE),\
    .value._string = NULL,\
    .oldvalue._tm = NULL,\
    .oldvalue._string = NULL

#define msfields(TYPE, NAME, VALUE) \
    .type = TYPE,\
    .name = #NAME,\
    .dirty = 0,\
    .value._short = 0,\
    .value._string = str_create(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
    .oldvalue._short = 0,\
    .oldvalue._string = NULL

#define mparam_bool(NAME, VALUE) { mnfields(MODEL_BOOL, _short, NAME, VALUE) }
#define mparam_smallint(NAME, VALUE) { mnfields(MODEL_SMALLINT, _short, NAME, VALUE) }
#define mparam_int(NAME, VALUE) { mnfields(MODEL_INT, _int, NAME, VALUE) }
#define mparam_bigint(NAME, VALUE) { mnfields(MODEL_BIGINT, _bigint, NAME, VALUE) }
#define mparam_float(NAME, VALUE) { mnfields(MODEL_FLOAT, _float, NAME, VALUE) }
#define mparam_double(NAME, VALUE) { mnfields(MODEL_DOUBLE, _double, NAME, VALUE) }
#define mparam_decimal(NAME, VALUE) { mnfields(MODEL_DECIMAL, _ldouble, NAME, VALUE) }
#define mparam_money(NAME, VALUE) { mnfields(MODEL_MONEY, _double, NAME, VALUE) }
#define mparam_date(NAME, VALUE) { mdfields(MODEL_DATE, NAME, VALUE) }
#define mparam_time(NAME, VALUE) { mdfields(MODEL_TIME, NAME, VALUE) }
#define mparam_timetz(NAME, VALUE) { mdfields(MODEL_TIMETZ, NAME, VALUE) }
#define mparam_timestamp(NAME, VALUE) { mdfields(MODEL_TIMESTAMP, NAME, VALUE) }
#define mparam_timestamptz(NAME, VALUE) { mdfields(MODEL_TIMESTAMPTZ, NAME, VALUE) }
#define mparam_json(NAME, VALUE) \
    {\
        .type = MODEL_JSON,\
        .name = #NAME,\
        .dirty = 0,\
        .value._jsondoc = json_create(VALUE),\
        .value._string = NULL,\
        .oldvalue._jsondoc = NULL,\
        .oldvalue._string = NULL\
    }

#define mparam_binary(NAME, VALUE) { msfields(MODEL_BINARY, NAME, VALUE) }
#define mparam_varchar(NAME, VALUE) { msfields(MODEL_VARCHAR, NAME, VALUE) }
#define mparam_char(NAME, VALUE) { msfields(MODEL_CHAR, NAME, VALUE) }
#define mparam_text(NAME, VALUE) { msfields(MODEL_TEXT, NAME, VALUE) }
#define mparam_enum(NAME, VALUE, ...) \
    {\
        .type = MODEL_ENUM,\
        .name = #NAME,\
        .dirty = 0,\
        .value._enum = enums_create(args_str(__VA_ARGS__)),\
        .value._string = str_create(VALUE, strlen(VALUE != NULL ? VALUE : "")),\
        .oldvalue._enum = NULL,\
        .oldvalue._string = NULL\
    }
#define mparam_array(NAME, VALUE) \
    {\
        .type = MODEL_ARRAY,\
        .name = #NAME,\
        .dirty = 0,\
        .value._array = VALUE,\
        .value._string = NULL,\
        .oldvalue._array = NULL,\
        .oldvalue._string = NULL\
    }



#define mfield_bool(NAME, VALUE) .NAME = mparam_bool(NAME, VALUE)
#define mfield_smallint(NAME, VALUE) .NAME = mparam_smallint(NAME, VALUE)
#define mfield_int(NAME, VALUE) .NAME = mparam_int(NAME, VALUE)
#define mfield_bigint(NAME, VALUE) .NAME = mparam_bigint(NAME, VALUE)
#define mfield_float(NAME, VALUE) .NAME = mparam_float(NAME, VALUE)
#define mfield_double(NAME, VALUE) .NAME = mparam_double(NAME, VALUE)
#define mfield_decimal(NAME, VALUE) .NAME = mparam_decimal(NAME, VALUE)
#define mfield_money(NAME, VALUE) .NAME = mparam_money(NAME, VALUE)
#define mfield_date(NAME, VALUE) .NAME = mparam_date(NAME, VALUE)
#define mfield_time(NAME, VALUE) .NAME = mparam_time(NAME, VALUE)
#define mfield_timetz(NAME, VALUE) .NAME = mparam_timetz(NAME, VALUE)
#define mfield_timestamp(NAME, VALUE) .NAME = mparam_timestamp(NAME, VALUE)
#define mfield_timestamptz(NAME, VALUE) .NAME = mparam_timestamptz(NAME, VALUE)
#define mfield_json(NAME, VALUE) .NAME = mparam_json(NAME, VALUE)
#define mfield_binary(NAME, VALUE) .NAME = mparam_binary(NAME, VALUE)
#define mfield_varchar(NAME, VALUE) .NAME = mparam_varchar(NAME, VALUE)
#define mfield_char(NAME, VALUE) .NAME = mparam_char(NAME, VALUE)
#define mfield_text(NAME, VALUE) .NAME = mparam_text(NAME, VALUE)
#define mfield_enum(NAME, VALUE, ...) .NAME = mparam_enum(NAME, VALUE, __VA_ARGS__)














#define MDL_VA_NARGS_REVERSE_(_441,_440,_439,_438,_437,_436,_435,_434,_433,_432,_431,_430,_429,_428,_427,_426,_425,_424,_423,_422,_421,_420,_419,_418,_417,_416,_415,_414,_413,_412,_411,_410,_409,_408,_407,_406,_405,_404,_403,_402,_401,_400,_399,_398,_397,_396,_395,_394,_393,_392,_391,_390,_389,_388,_387,_386,_385,_384,_383,_382,_381,_380,_379,_378,_377,_376,_375,_374,_373,_372,_371,_370,_369,_368,_367,_366,_365,_364,_363,_362,_361,_360,_359,_358,_357,_356,_355,_354,_353,_352,_351,_350,_349,_348,_347,_346,_345,_344,_343,_342,_341,_340,_339,_338,_337,_336,_335,_334,_333,_332,_331,_330,_329,_328,_327,_326,_325,_324,_323,_322,_321,_320,_319,_318,_317,_316,_315,_314,_313,_312,_311,_310,_309,_308,_307,_306,_305,_304,_303,_302,_301,_300,_299,_298,_297,_296,_295,_294,_293,_292,_291,_290,_289,_288,_287,_286,_285,_284,_283,_282,_281,_280,_279,_278,_277,_276,_275,_274,_273,_272,_271,_270,_269,_268,_267,_266,_265,_264,_263,_262,_261,_260,_259,_258,_257,_256,_255,_254,_253,_252,_251,_250,_249,_248,_247,_246,_245,_244,_243,_242,_241,_240,_239,_238,_237,_236,_235,_234,_233,_232,_231,_230,_229,_228,_227,_226,_225,_224,_223,_222,_221,_220,_219,_218,_217,_216,_215,_214,_213,_212,_211,_210,_209,_208,_207,_206,_205,_204,_203,_202,_201,_200,_199,_198,_197,_196,_195,_194,_193,_192,_191,_190,_189,_188,_187,_186,_185,_184,_183,_182,_181,_180,_179,_178,_177,_176,_175,_174,_173,_172,_171,_170,_169,_168,_167,_166,_165,_164,_163,_162,_161,_160,_159,_158,_157,_156,_155,_154,_153,_152,_151,_150,_149,_148,_147,_146,_145,_144,_143,_142,_141,_140,_139,_138,_137,_136,_135,_134,_133,_132,_131,_130,_129,_128,_127,_126,_125,_124,_123,_122,_121,_120,_119,_118,_117,_116,_115,_114,_113,_112,_111,_110,_109,_108,_107,_106,_105,_104,_103,_102,_101,_100,_99,_98,_97,_96,_95,_94,_93,_92,_91,_90,_89,_88,_87,_86,_85,_84,_83,_82,_81,_80,_79,_78,_77,_76,_75,_74,_73,_72,_71,_70,_69,_68,_67,_66,_65,_64,_63,_62,_61,_60,_59,_58,_57,_56,_55,_54,_53,_52,_51,_50,_49,_48,_47,_46,_45,_44,_43,_42,_41,_40,_39,_38,_37,_36,_35,_34,_33,_32,_31,_30,_29,_28,_27,_26,_25,_24,_23,_22,_21,_20,_19,_18,_17,_16,_15,_14,_13,_12,_11,_10,_9,_8,_7,_6,_5,_4,_3,_2,_1,N,...) N
#define MDL_VA_NARGS(...) MDL_VA_NARGS_REVERSE_(__VA_ARGS__, 441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,401,400,399,398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,380,379,378,377,376,375,374,373,372,371,370,369,368,367,366,365,364,363,362,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,330,329,328,327,326,325,324,323,322,321,320,319,318,317,316,315,314,313,312,311,310,309,308,307,306,305,304,303,302,301,300,299,298,297,296,295,294,293,292,291,290,289,288,287,286,285,284,283,282,281,280,279,278,277,276,275,274,273,272,271,270,269,268,267,266,265,264,263,262,261,260,259,258,257,256,255,254,253,252,251,250,249,248,247,246,245,244,243,242,241,240,239,238,237,236,235,234,233,232,231,230,229,228,227,226,225,224,223,222,221,220,219,218,217,216,215,214,213,212,211,210,209,208,207,206,205,204,203,202,201,200,199,198,197,196,195,194,193,192,191,190,189,188,187,186,185,184,183,182,181,180,179,178,177,176,175,174,173,172,171,170,169,168,167,166,165,164,163,162,161,160,159,158,157,156,155,154,153,152,151,150,149,148,147,146,145,144,143,142,141,140,139,138,137,136,135,134,133,132,131,130,129,128,127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define MDL_ITEM_CONCAT(A, B) A##B
#define MDL_ITEM(ARRAY, N, ...) MDL_ITEM_CONCAT(MDL_ITEM_, N)(ARRAY, __VA_ARGS__)
#define MDL_ITEM_1(ARRAY, NAME) NAME, NULL, model_param_clear));
#define MDL_ITEM_2(ARRAY, NAME, ...)  NAME, MDL_ITEM_1(ARRAY, __VA_ARGS__)
#define MDL_ITEM_3(ARRAY, NAME, ...) NAME, MDL_ITEM_2(ARRAY, __VA_ARGS__)
#define MDL_ITEM_4(ARRAY, NAME, ...) NAME, MDL_ITEM_3(ARRAY, __VA_ARGS__)
#define MDL_ITEM_5(ARRAY, NAME, ...) NAME, MDL_ITEM_4(ARRAY, __VA_ARGS__)
#define MDL_ITEM_6(ARRAY, NAME, ...) NAME, MDL_ITEM_5(ARRAY, __VA_ARGS__)
#define MDL_ITEM_7(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_6(ARRAY, __VA_ARGS__)

#define MDL_ITEM_8(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_7(ARRAY, __VA_ARGS__)
#define MDL_ITEM_9(ARRAY, NAME, ...)  NAME, MDL_ITEM_8(ARRAY, __VA_ARGS__)
#define MDL_ITEM_10(ARRAY, NAME, ...) NAME, MDL_ITEM_9(ARRAY, __VA_ARGS__)
#define MDL_ITEM_11(ARRAY, NAME, ...) NAME, MDL_ITEM_10(ARRAY, __VA_ARGS__)
#define MDL_ITEM_12(ARRAY, NAME, ...) NAME, MDL_ITEM_11(ARRAY, __VA_ARGS__)
#define MDL_ITEM_13(ARRAY, NAME, ...) NAME, MDL_ITEM_12(ARRAY, __VA_ARGS__)
#define MDL_ITEM_14(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_13(ARRAY, __VA_ARGS__)

#define MDL_ITEM_15(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_14(ARRAY, __VA_ARGS__)
#define MDL_ITEM_16(ARRAY, NAME, ...)  NAME, MDL_ITEM_15(ARRAY, __VA_ARGS__)
#define MDL_ITEM_17(ARRAY, NAME, ...) NAME, MDL_ITEM_16(ARRAY, __VA_ARGS__)
#define MDL_ITEM_18(ARRAY, NAME, ...) NAME, MDL_ITEM_17(ARRAY, __VA_ARGS__)
#define MDL_ITEM_19(ARRAY, NAME, ...) NAME, MDL_ITEM_18(ARRAY, __VA_ARGS__)
#define MDL_ITEM_20(ARRAY, NAME, ...) NAME, MDL_ITEM_19(ARRAY, __VA_ARGS__)
#define MDL_ITEM_21(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_20(ARRAY, __VA_ARGS__)

#define MDL_ITEM_22(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_21(ARRAY, __VA_ARGS__)
#define MDL_ITEM_23(ARRAY, NAME, ...)  NAME, MDL_ITEM_22(ARRAY, __VA_ARGS__)
#define MDL_ITEM_24(ARRAY, NAME, ...) NAME, MDL_ITEM_23(ARRAY, __VA_ARGS__)
#define MDL_ITEM_25(ARRAY, NAME, ...) NAME, MDL_ITEM_24(ARRAY, __VA_ARGS__)
#define MDL_ITEM_26(ARRAY, NAME, ...) NAME, MDL_ITEM_25(ARRAY, __VA_ARGS__)
#define MDL_ITEM_27(ARRAY, NAME, ...) NAME, MDL_ITEM_26(ARRAY, __VA_ARGS__)
#define MDL_ITEM_28(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_27(ARRAY, __VA_ARGS__)

#define MDL_ITEM_29(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_28(ARRAY, __VA_ARGS__)
#define MDL_ITEM_30(ARRAY, NAME, ...)  NAME, MDL_ITEM_29(ARRAY, __VA_ARGS__)
#define MDL_ITEM_31(ARRAY, NAME, ...) NAME, MDL_ITEM_30(ARRAY, __VA_ARGS__)
#define MDL_ITEM_32(ARRAY, NAME, ...) NAME, MDL_ITEM_31(ARRAY, __VA_ARGS__)
#define MDL_ITEM_33(ARRAY, NAME, ...) NAME, MDL_ITEM_32(ARRAY, __VA_ARGS__)
#define MDL_ITEM_34(ARRAY, NAME, ...) NAME, MDL_ITEM_33(ARRAY, __VA_ARGS__)
#define MDL_ITEM_35(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_34(ARRAY, __VA_ARGS__)

#define MDL_ITEM_36(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_35(ARRAY, __VA_ARGS__)
#define MDL_ITEM_37(ARRAY, NAME, ...)  NAME, MDL_ITEM_36(ARRAY, __VA_ARGS__)
#define MDL_ITEM_38(ARRAY, NAME, ...) NAME, MDL_ITEM_37(ARRAY, __VA_ARGS__)
#define MDL_ITEM_39(ARRAY, NAME, ...) NAME, MDL_ITEM_38(ARRAY, __VA_ARGS__)
#define MDL_ITEM_40(ARRAY, NAME, ...) NAME, MDL_ITEM_39(ARRAY, __VA_ARGS__)
#define MDL_ITEM_41(ARRAY, NAME, ...) NAME, MDL_ITEM_40(ARRAY, __VA_ARGS__)
#define MDL_ITEM_42(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_41(ARRAY, __VA_ARGS__)

#define MDL_ITEM_43(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_42(ARRAY, __VA_ARGS__)
#define MDL_ITEM_44(ARRAY, NAME, ...)  NAME, MDL_ITEM_43(ARRAY, __VA_ARGS__)
#define MDL_ITEM_45(ARRAY, NAME, ...) NAME, MDL_ITEM_44(ARRAY, __VA_ARGS__)
#define MDL_ITEM_46(ARRAY, NAME, ...) NAME, MDL_ITEM_45(ARRAY, __VA_ARGS__)
#define MDL_ITEM_47(ARRAY, NAME, ...) NAME, MDL_ITEM_46(ARRAY, __VA_ARGS__)
#define MDL_ITEM_48(ARRAY, NAME, ...) NAME, MDL_ITEM_47(ARRAY, __VA_ARGS__)
#define MDL_ITEM_49(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_48(ARRAY, __VA_ARGS__)

#define MDL_ITEM_50(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_49(ARRAY, __VA_ARGS__)
#define MDL_ITEM_51(ARRAY, NAME, ...)  NAME, MDL_ITEM_50(ARRAY, __VA_ARGS__)
#define MDL_ITEM_52(ARRAY, NAME, ...) NAME, MDL_ITEM_51(ARRAY, __VA_ARGS__)
#define MDL_ITEM_53(ARRAY, NAME, ...) NAME, MDL_ITEM_52(ARRAY, __VA_ARGS__)
#define MDL_ITEM_54(ARRAY, NAME, ...) NAME, MDL_ITEM_53(ARRAY, __VA_ARGS__)
#define MDL_ITEM_55(ARRAY, NAME, ...) NAME, MDL_ITEM_54(ARRAY, __VA_ARGS__)
#define MDL_ITEM_56(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_55(ARRAY, __VA_ARGS__)

#define MDL_ITEM_57(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_56(ARRAY, __VA_ARGS__)
#define MDL_ITEM_58(ARRAY, NAME, ...)  NAME, MDL_ITEM_57(ARRAY, __VA_ARGS__)
#define MDL_ITEM_59(ARRAY, NAME, ...) NAME, MDL_ITEM_58(ARRAY, __VA_ARGS__)
#define MDL_ITEM_60(ARRAY, NAME, ...) NAME, MDL_ITEM_59(ARRAY, __VA_ARGS__)
#define MDL_ITEM_61(ARRAY, NAME, ...) NAME, MDL_ITEM_60(ARRAY, __VA_ARGS__)
#define MDL_ITEM_62(ARRAY, NAME, ...) NAME, MDL_ITEM_61(ARRAY, __VA_ARGS__)
#define MDL_ITEM_63(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_62(ARRAY, __VA_ARGS__)

#define MDL_ITEM_64(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_63(ARRAY, __VA_ARGS__)
#define MDL_ITEM_65(ARRAY, NAME, ...)  NAME, MDL_ITEM_64(ARRAY, __VA_ARGS__)
#define MDL_ITEM_66(ARRAY, NAME, ...) NAME, MDL_ITEM_65(ARRAY, __VA_ARGS__)
#define MDL_ITEM_67(ARRAY, NAME, ...) NAME, MDL_ITEM_66(ARRAY, __VA_ARGS__)
#define MDL_ITEM_68(ARRAY, NAME, ...) NAME, MDL_ITEM_67(ARRAY, __VA_ARGS__)
#define MDL_ITEM_69(ARRAY, NAME, ...) NAME, MDL_ITEM_68(ARRAY, __VA_ARGS__)
#define MDL_ITEM_70(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_69(ARRAY, __VA_ARGS__)

#define MDL_ITEM_71(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_70(ARRAY, __VA_ARGS__)
#define MDL_ITEM_72(ARRAY, NAME, ...)  NAME, MDL_ITEM_71(ARRAY, __VA_ARGS__)
#define MDL_ITEM_73(ARRAY, NAME, ...) NAME, MDL_ITEM_72(ARRAY, __VA_ARGS__)
#define MDL_ITEM_74(ARRAY, NAME, ...) NAME, MDL_ITEM_73(ARRAY, __VA_ARGS__)
#define MDL_ITEM_75(ARRAY, NAME, ...) NAME, MDL_ITEM_74(ARRAY, __VA_ARGS__)
#define MDL_ITEM_76(ARRAY, NAME, ...) NAME, MDL_ITEM_75(ARRAY, __VA_ARGS__)
#define MDL_ITEM_77(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_76(ARRAY, __VA_ARGS__)

#define MDL_ITEM_78(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_77(ARRAY, __VA_ARGS__)
#define MDL_ITEM_79(ARRAY, NAME, ...)  NAME, MDL_ITEM_78(ARRAY, __VA_ARGS__)
#define MDL_ITEM_80(ARRAY, NAME, ...) NAME, MDL_ITEM_79(ARRAY, __VA_ARGS__)
#define MDL_ITEM_81(ARRAY, NAME, ...) NAME, MDL_ITEM_80(ARRAY, __VA_ARGS__)
#define MDL_ITEM_82(ARRAY, NAME, ...) NAME, MDL_ITEM_81(ARRAY, __VA_ARGS__)
#define MDL_ITEM_83(ARRAY, NAME, ...) NAME, MDL_ITEM_82(ARRAY, __VA_ARGS__)
#define MDL_ITEM_84(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_83(ARRAY, __VA_ARGS__)

#define MDL_ITEM_85(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_84(ARRAY, __VA_ARGS__)
#define MDL_ITEM_86(ARRAY, NAME, ...)  NAME, MDL_ITEM_85(ARRAY, __VA_ARGS__)
#define MDL_ITEM_87(ARRAY, NAME, ...) NAME, MDL_ITEM_86(ARRAY, __VA_ARGS__)
#define MDL_ITEM_88(ARRAY, NAME, ...) NAME, MDL_ITEM_87(ARRAY, __VA_ARGS__)
#define MDL_ITEM_89(ARRAY, NAME, ...) NAME, MDL_ITEM_88(ARRAY, __VA_ARGS__)
#define MDL_ITEM_90(ARRAY, NAME, ...) NAME, MDL_ITEM_89(ARRAY, __VA_ARGS__)
#define MDL_ITEM_91(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_90(ARRAY, __VA_ARGS__)

#define MDL_ITEM_92(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_91(ARRAY, __VA_ARGS__)
#define MDL_ITEM_93(ARRAY, NAME, ...)  NAME, MDL_ITEM_92(ARRAY, __VA_ARGS__)
#define MDL_ITEM_94(ARRAY, NAME, ...) NAME, MDL_ITEM_93(ARRAY, __VA_ARGS__)
#define MDL_ITEM_95(ARRAY, NAME, ...) NAME, MDL_ITEM_94(ARRAY, __VA_ARGS__)
#define MDL_ITEM_96(ARRAY, NAME, ...) NAME, MDL_ITEM_95(ARRAY, __VA_ARGS__)
#define MDL_ITEM_97(ARRAY, NAME, ...) NAME, MDL_ITEM_96(ARRAY, __VA_ARGS__)
#define MDL_ITEM_98(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_97(ARRAY, __VA_ARGS__)

#define MDL_ITEM_99(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_98(ARRAY, __VA_ARGS__)
#define MDL_ITEM_100(ARRAY, NAME, ...)  NAME, MDL_ITEM_99(ARRAY, __VA_ARGS__)
#define MDL_ITEM_101(ARRAY, NAME, ...) NAME, MDL_ITEM_100(ARRAY, __VA_ARGS__)
#define MDL_ITEM_102(ARRAY, NAME, ...) NAME, MDL_ITEM_101(ARRAY, __VA_ARGS__)
#define MDL_ITEM_103(ARRAY, NAME, ...) NAME, MDL_ITEM_102(ARRAY, __VA_ARGS__)
#define MDL_ITEM_104(ARRAY, NAME, ...) NAME, MDL_ITEM_103(ARRAY, __VA_ARGS__)
#define MDL_ITEM_105(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_104(ARRAY, __VA_ARGS__)

#define MDL_ITEM_106(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_105(ARRAY, __VA_ARGS__)
#define MDL_ITEM_107(ARRAY, NAME, ...)  NAME, MDL_ITEM_106(ARRAY, __VA_ARGS__)
#define MDL_ITEM_108(ARRAY, NAME, ...) NAME, MDL_ITEM_107(ARRAY, __VA_ARGS__)
#define MDL_ITEM_109(ARRAY, NAME, ...) NAME, MDL_ITEM_108(ARRAY, __VA_ARGS__)
#define MDL_ITEM_110(ARRAY, NAME, ...) NAME, MDL_ITEM_109(ARRAY, __VA_ARGS__)
#define MDL_ITEM_111(ARRAY, NAME, ...) NAME, MDL_ITEM_110(ARRAY, __VA_ARGS__)
#define MDL_ITEM_112(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_111(ARRAY, __VA_ARGS__)

#define MDL_ITEM_113(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_112(ARRAY, __VA_ARGS__)
#define MDL_ITEM_114(ARRAY, NAME, ...)  NAME, MDL_ITEM_113(ARRAY, __VA_ARGS__)
#define MDL_ITEM_115(ARRAY, NAME, ...) NAME, MDL_ITEM_114(ARRAY, __VA_ARGS__)
#define MDL_ITEM_116(ARRAY, NAME, ...) NAME, MDL_ITEM_115(ARRAY, __VA_ARGS__)
#define MDL_ITEM_117(ARRAY, NAME, ...) NAME, MDL_ITEM_116(ARRAY, __VA_ARGS__)
#define MDL_ITEM_118(ARRAY, NAME, ...) NAME, MDL_ITEM_117(ARRAY, __VA_ARGS__)
#define MDL_ITEM_119(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_118(ARRAY, __VA_ARGS__)

#define MDL_ITEM_120(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_119(ARRAY, __VA_ARGS__)
#define MDL_ITEM_121(ARRAY, NAME, ...)  NAME, MDL_ITEM_120(ARRAY, __VA_ARGS__)
#define MDL_ITEM_122(ARRAY, NAME, ...) NAME, MDL_ITEM_121(ARRAY, __VA_ARGS__)
#define MDL_ITEM_123(ARRAY, NAME, ...) NAME, MDL_ITEM_122(ARRAY, __VA_ARGS__)
#define MDL_ITEM_124(ARRAY, NAME, ...) NAME, MDL_ITEM_123(ARRAY, __VA_ARGS__)
#define MDL_ITEM_125(ARRAY, NAME, ...) NAME, MDL_ITEM_124(ARRAY, __VA_ARGS__)
#define MDL_ITEM_126(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_125(ARRAY, __VA_ARGS__)

#define MDL_ITEM_127(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_126(ARRAY, __VA_ARGS__)
#define MDL_ITEM_128(ARRAY, NAME, ...)  NAME, MDL_ITEM_127(ARRAY, __VA_ARGS__)
#define MDL_ITEM_129(ARRAY, NAME, ...) NAME, MDL_ITEM_128(ARRAY, __VA_ARGS__)
#define MDL_ITEM_130(ARRAY, NAME, ...) NAME, MDL_ITEM_129(ARRAY, __VA_ARGS__)
#define MDL_ITEM_131(ARRAY, NAME, ...) NAME, MDL_ITEM_130(ARRAY, __VA_ARGS__)
#define MDL_ITEM_132(ARRAY, NAME, ...) NAME, MDL_ITEM_131(ARRAY, __VA_ARGS__)
#define MDL_ITEM_133(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_132(ARRAY, __VA_ARGS__)

#define MDL_ITEM_134(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_133(ARRAY, __VA_ARGS__)
#define MDL_ITEM_135(ARRAY, NAME, ...)  NAME, MDL_ITEM_134(ARRAY, __VA_ARGS__)
#define MDL_ITEM_136(ARRAY, NAME, ...) NAME, MDL_ITEM_135(ARRAY, __VA_ARGS__)
#define MDL_ITEM_137(ARRAY, NAME, ...) NAME, MDL_ITEM_136(ARRAY, __VA_ARGS__)
#define MDL_ITEM_138(ARRAY, NAME, ...) NAME, MDL_ITEM_137(ARRAY, __VA_ARGS__)
#define MDL_ITEM_139(ARRAY, NAME, ...) NAME, MDL_ITEM_138(ARRAY, __VA_ARGS__)
#define MDL_ITEM_140(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_139(ARRAY, __VA_ARGS__)

#define MDL_ITEM_141(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_140(ARRAY, __VA_ARGS__)
#define MDL_ITEM_142(ARRAY, NAME, ...)  NAME, MDL_ITEM_141(ARRAY, __VA_ARGS__)
#define MDL_ITEM_143(ARRAY, NAME, ...) NAME, MDL_ITEM_142(ARRAY, __VA_ARGS__)
#define MDL_ITEM_144(ARRAY, NAME, ...) NAME, MDL_ITEM_143(ARRAY, __VA_ARGS__)
#define MDL_ITEM_145(ARRAY, NAME, ...) NAME, MDL_ITEM_144(ARRAY, __VA_ARGS__)
#define MDL_ITEM_146(ARRAY, NAME, ...) NAME, MDL_ITEM_145(ARRAY, __VA_ARGS__)
#define MDL_ITEM_147(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_146(ARRAY, __VA_ARGS__)

#define MDL_ITEM_148(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_147(ARRAY, __VA_ARGS__)
#define MDL_ITEM_149(ARRAY, NAME, ...)  NAME, MDL_ITEM_148(ARRAY, __VA_ARGS__)
#define MDL_ITEM_150(ARRAY, NAME, ...) NAME, MDL_ITEM_149(ARRAY, __VA_ARGS__)
#define MDL_ITEM_151(ARRAY, NAME, ...) NAME, MDL_ITEM_150(ARRAY, __VA_ARGS__)
#define MDL_ITEM_152(ARRAY, NAME, ...) NAME, MDL_ITEM_151(ARRAY, __VA_ARGS__)
#define MDL_ITEM_153(ARRAY, NAME, ...) NAME, MDL_ITEM_152(ARRAY, __VA_ARGS__)
#define MDL_ITEM_154(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_153(ARRAY, __VA_ARGS__)

#define MDL_ITEM_155(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_154(ARRAY, __VA_ARGS__)
#define MDL_ITEM_156(ARRAY, NAME, ...)  NAME, MDL_ITEM_155(ARRAY, __VA_ARGS__)
#define MDL_ITEM_157(ARRAY, NAME, ...) NAME, MDL_ITEM_156(ARRAY, __VA_ARGS__)
#define MDL_ITEM_158(ARRAY, NAME, ...) NAME, MDL_ITEM_157(ARRAY, __VA_ARGS__)
#define MDL_ITEM_159(ARRAY, NAME, ...) NAME, MDL_ITEM_158(ARRAY, __VA_ARGS__)
#define MDL_ITEM_160(ARRAY, NAME, ...) NAME, MDL_ITEM_159(ARRAY, __VA_ARGS__)
#define MDL_ITEM_161(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_160(ARRAY, __VA_ARGS__)

#define MDL_ITEM_162(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_161(ARRAY, __VA_ARGS__)
#define MDL_ITEM_163(ARRAY, NAME, ...)  NAME, MDL_ITEM_162(ARRAY, __VA_ARGS__)
#define MDL_ITEM_164(ARRAY, NAME, ...) NAME, MDL_ITEM_163(ARRAY, __VA_ARGS__)
#define MDL_ITEM_165(ARRAY, NAME, ...) NAME, MDL_ITEM_164(ARRAY, __VA_ARGS__)
#define MDL_ITEM_166(ARRAY, NAME, ...) NAME, MDL_ITEM_165(ARRAY, __VA_ARGS__)
#define MDL_ITEM_167(ARRAY, NAME, ...) NAME, MDL_ITEM_166(ARRAY, __VA_ARGS__)
#define MDL_ITEM_168(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_167(ARRAY, __VA_ARGS__)

#define MDL_ITEM_169(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_168(ARRAY, __VA_ARGS__)
#define MDL_ITEM_170(ARRAY, NAME, ...)  NAME, MDL_ITEM_169(ARRAY, __VA_ARGS__)
#define MDL_ITEM_171(ARRAY, NAME, ...) NAME, MDL_ITEM_170(ARRAY, __VA_ARGS__)
#define MDL_ITEM_172(ARRAY, NAME, ...) NAME, MDL_ITEM_171(ARRAY, __VA_ARGS__)
#define MDL_ITEM_173(ARRAY, NAME, ...) NAME, MDL_ITEM_172(ARRAY, __VA_ARGS__)
#define MDL_ITEM_174(ARRAY, NAME, ...) NAME, MDL_ITEM_173(ARRAY, __VA_ARGS__)
#define MDL_ITEM_175(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_174(ARRAY, __VA_ARGS__)

#define MDL_ITEM_176(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_175(ARRAY, __VA_ARGS__)
#define MDL_ITEM_177(ARRAY, NAME, ...)  NAME, MDL_ITEM_176(ARRAY, __VA_ARGS__)
#define MDL_ITEM_178(ARRAY, NAME, ...) NAME, MDL_ITEM_177(ARRAY, __VA_ARGS__)
#define MDL_ITEM_179(ARRAY, NAME, ...) NAME, MDL_ITEM_178(ARRAY, __VA_ARGS__)
#define MDL_ITEM_180(ARRAY, NAME, ...) NAME, MDL_ITEM_179(ARRAY, __VA_ARGS__)
#define MDL_ITEM_181(ARRAY, NAME, ...) NAME, MDL_ITEM_180(ARRAY, __VA_ARGS__)
#define MDL_ITEM_182(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_181(ARRAY, __VA_ARGS__)

#define MDL_ITEM_183(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_182(ARRAY, __VA_ARGS__)
#define MDL_ITEM_184(ARRAY, NAME, ...)  NAME, MDL_ITEM_183(ARRAY, __VA_ARGS__)
#define MDL_ITEM_185(ARRAY, NAME, ...) NAME, MDL_ITEM_184(ARRAY, __VA_ARGS__)
#define MDL_ITEM_186(ARRAY, NAME, ...) NAME, MDL_ITEM_185(ARRAY, __VA_ARGS__)
#define MDL_ITEM_187(ARRAY, NAME, ...) NAME, MDL_ITEM_186(ARRAY, __VA_ARGS__)
#define MDL_ITEM_188(ARRAY, NAME, ...) NAME, MDL_ITEM_187(ARRAY, __VA_ARGS__)
#define MDL_ITEM_189(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_188(ARRAY, __VA_ARGS__)

#define MDL_ITEM_190(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_189(ARRAY, __VA_ARGS__)
#define MDL_ITEM_191(ARRAY, NAME, ...)  NAME, MDL_ITEM_190(ARRAY, __VA_ARGS__)
#define MDL_ITEM_192(ARRAY, NAME, ...) NAME, MDL_ITEM_191(ARRAY, __VA_ARGS__)
#define MDL_ITEM_193(ARRAY, NAME, ...) NAME, MDL_ITEM_192(ARRAY, __VA_ARGS__)
#define MDL_ITEM_194(ARRAY, NAME, ...) NAME, MDL_ITEM_193(ARRAY, __VA_ARGS__)
#define MDL_ITEM_195(ARRAY, NAME, ...) NAME, MDL_ITEM_194(ARRAY, __VA_ARGS__)
#define MDL_ITEM_196(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_195(ARRAY, __VA_ARGS__)

#define MDL_ITEM_197(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_196(ARRAY, __VA_ARGS__)
#define MDL_ITEM_198(ARRAY, NAME, ...)  NAME, MDL_ITEM_197(ARRAY, __VA_ARGS__)
#define MDL_ITEM_199(ARRAY, NAME, ...) NAME, MDL_ITEM_198(ARRAY, __VA_ARGS__)
#define MDL_ITEM_200(ARRAY, NAME, ...) NAME, MDL_ITEM_199(ARRAY, __VA_ARGS__)
#define MDL_ITEM_201(ARRAY, NAME, ...) NAME, MDL_ITEM_200(ARRAY, __VA_ARGS__)
#define MDL_ITEM_202(ARRAY, NAME, ...) NAME, MDL_ITEM_201(ARRAY, __VA_ARGS__)
#define MDL_ITEM_203(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_202(ARRAY, __VA_ARGS__)

#define MDL_ITEM_204(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_203(ARRAY, __VA_ARGS__)
#define MDL_ITEM_205(ARRAY, NAME, ...)  NAME, MDL_ITEM_204(ARRAY, __VA_ARGS__)
#define MDL_ITEM_206(ARRAY, NAME, ...) NAME, MDL_ITEM_205(ARRAY, __VA_ARGS__)
#define MDL_ITEM_207(ARRAY, NAME, ...) NAME, MDL_ITEM_206(ARRAY, __VA_ARGS__)
#define MDL_ITEM_208(ARRAY, NAME, ...) NAME, MDL_ITEM_207(ARRAY, __VA_ARGS__)
#define MDL_ITEM_209(ARRAY, NAME, ...) NAME, MDL_ITEM_208(ARRAY, __VA_ARGS__)
#define MDL_ITEM_210(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_209(ARRAY, __VA_ARGS__)

#define MDL_ITEM_211(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_210(ARRAY, __VA_ARGS__)
#define MDL_ITEM_212(ARRAY, NAME, ...)  NAME, MDL_ITEM_211(ARRAY, __VA_ARGS__)
#define MDL_ITEM_213(ARRAY, NAME, ...) NAME, MDL_ITEM_212(ARRAY, __VA_ARGS__)
#define MDL_ITEM_214(ARRAY, NAME, ...) NAME, MDL_ITEM_213(ARRAY, __VA_ARGS__)
#define MDL_ITEM_215(ARRAY, NAME, ...) NAME, MDL_ITEM_214(ARRAY, __VA_ARGS__)
#define MDL_ITEM_216(ARRAY, NAME, ...) NAME, MDL_ITEM_215(ARRAY, __VA_ARGS__)
#define MDL_ITEM_217(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_216(ARRAY, __VA_ARGS__)

#define MDL_ITEM_218(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_217(ARRAY, __VA_ARGS__)
#define MDL_ITEM_219(ARRAY, NAME, ...)  NAME, MDL_ITEM_218(ARRAY, __VA_ARGS__)
#define MDL_ITEM_220(ARRAY, NAME, ...) NAME, MDL_ITEM_219(ARRAY, __VA_ARGS__)
#define MDL_ITEM_221(ARRAY, NAME, ...) NAME, MDL_ITEM_220(ARRAY, __VA_ARGS__)
#define MDL_ITEM_222(ARRAY, NAME, ...) NAME, MDL_ITEM_221(ARRAY, __VA_ARGS__)
#define MDL_ITEM_223(ARRAY, NAME, ...) NAME, MDL_ITEM_222(ARRAY, __VA_ARGS__)
#define MDL_ITEM_224(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_223(ARRAY, __VA_ARGS__)

#define MDL_ITEM_225(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_224(ARRAY, __VA_ARGS__)
#define MDL_ITEM_226(ARRAY, NAME, ...)  NAME, MDL_ITEM_225(ARRAY, __VA_ARGS__)
#define MDL_ITEM_227(ARRAY, NAME, ...) NAME, MDL_ITEM_226(ARRAY, __VA_ARGS__)
#define MDL_ITEM_228(ARRAY, NAME, ...) NAME, MDL_ITEM_227(ARRAY, __VA_ARGS__)
#define MDL_ITEM_229(ARRAY, NAME, ...) NAME, MDL_ITEM_228(ARRAY, __VA_ARGS__)
#define MDL_ITEM_230(ARRAY, NAME, ...) NAME, MDL_ITEM_229(ARRAY, __VA_ARGS__)
#define MDL_ITEM_231(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_230(ARRAY, __VA_ARGS__)

#define MDL_ITEM_232(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_231(ARRAY, __VA_ARGS__)
#define MDL_ITEM_233(ARRAY, NAME, ...)  NAME, MDL_ITEM_232(ARRAY, __VA_ARGS__)
#define MDL_ITEM_234(ARRAY, NAME, ...) NAME, MDL_ITEM_233(ARRAY, __VA_ARGS__)
#define MDL_ITEM_235(ARRAY, NAME, ...) NAME, MDL_ITEM_234(ARRAY, __VA_ARGS__)
#define MDL_ITEM_236(ARRAY, NAME, ...) NAME, MDL_ITEM_235(ARRAY, __VA_ARGS__)
#define MDL_ITEM_237(ARRAY, NAME, ...) NAME, MDL_ITEM_236(ARRAY, __VA_ARGS__)
#define MDL_ITEM_238(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_237(ARRAY, __VA_ARGS__)

#define MDL_ITEM_239(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_238(ARRAY, __VA_ARGS__)
#define MDL_ITEM_240(ARRAY, NAME, ...)  NAME, MDL_ITEM_239(ARRAY, __VA_ARGS__)
#define MDL_ITEM_241(ARRAY, NAME, ...) NAME, MDL_ITEM_240(ARRAY, __VA_ARGS__)
#define MDL_ITEM_242(ARRAY, NAME, ...) NAME, MDL_ITEM_241(ARRAY, __VA_ARGS__)
#define MDL_ITEM_243(ARRAY, NAME, ...) NAME, MDL_ITEM_242(ARRAY, __VA_ARGS__)
#define MDL_ITEM_244(ARRAY, NAME, ...) NAME, MDL_ITEM_243(ARRAY, __VA_ARGS__)
#define MDL_ITEM_245(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_244(ARRAY, __VA_ARGS__)

#define MDL_ITEM_246(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_245(ARRAY, __VA_ARGS__)
#define MDL_ITEM_247(ARRAY, NAME, ...)  NAME, MDL_ITEM_246(ARRAY, __VA_ARGS__)
#define MDL_ITEM_248(ARRAY, NAME, ...) NAME, MDL_ITEM_247(ARRAY, __VA_ARGS__)
#define MDL_ITEM_249(ARRAY, NAME, ...) NAME, MDL_ITEM_248(ARRAY, __VA_ARGS__)
#define MDL_ITEM_250(ARRAY, NAME, ...) NAME, MDL_ITEM_249(ARRAY, __VA_ARGS__)
#define MDL_ITEM_251(ARRAY, NAME, ...) NAME, MDL_ITEM_250(ARRAY, __VA_ARGS__)
#define MDL_ITEM_252(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_251(ARRAY, __VA_ARGS__)

#define MDL_ITEM_253(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_252(ARRAY, __VA_ARGS__)
#define MDL_ITEM_254(ARRAY, NAME, ...)  NAME, MDL_ITEM_253(ARRAY, __VA_ARGS__)
#define MDL_ITEM_255(ARRAY, NAME, ...) NAME, MDL_ITEM_254(ARRAY, __VA_ARGS__)
#define MDL_ITEM_256(ARRAY, NAME, ...) NAME, MDL_ITEM_255(ARRAY, __VA_ARGS__)
#define MDL_ITEM_257(ARRAY, NAME, ...) NAME, MDL_ITEM_256(ARRAY, __VA_ARGS__)
#define MDL_ITEM_258(ARRAY, NAME, ...) NAME, MDL_ITEM_257(ARRAY, __VA_ARGS__)
#define MDL_ITEM_259(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_258(ARRAY, __VA_ARGS__)

#define MDL_ITEM_260(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_259(ARRAY, __VA_ARGS__)
#define MDL_ITEM_261(ARRAY, NAME, ...)  NAME, MDL_ITEM_260(ARRAY, __VA_ARGS__)
#define MDL_ITEM_262(ARRAY, NAME, ...) NAME, MDL_ITEM_261(ARRAY, __VA_ARGS__)
#define MDL_ITEM_263(ARRAY, NAME, ...) NAME, MDL_ITEM_262(ARRAY, __VA_ARGS__)
#define MDL_ITEM_264(ARRAY, NAME, ...) NAME, MDL_ITEM_263(ARRAY, __VA_ARGS__)
#define MDL_ITEM_265(ARRAY, NAME, ...) NAME, MDL_ITEM_264(ARRAY, __VA_ARGS__)
#define MDL_ITEM_266(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_265(ARRAY, __VA_ARGS__)

#define MDL_ITEM_267(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_266(ARRAY, __VA_ARGS__)
#define MDL_ITEM_268(ARRAY, NAME, ...)  NAME, MDL_ITEM_267(ARRAY, __VA_ARGS__)
#define MDL_ITEM_269(ARRAY, NAME, ...) NAME, MDL_ITEM_268(ARRAY, __VA_ARGS__)
#define MDL_ITEM_270(ARRAY, NAME, ...) NAME, MDL_ITEM_269(ARRAY, __VA_ARGS__)
#define MDL_ITEM_271(ARRAY, NAME, ...) NAME, MDL_ITEM_270(ARRAY, __VA_ARGS__)
#define MDL_ITEM_272(ARRAY, NAME, ...) NAME, MDL_ITEM_271(ARRAY, __VA_ARGS__)
#define MDL_ITEM_273(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_272(ARRAY, __VA_ARGS__)

#define MDL_ITEM_274(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_273(ARRAY, __VA_ARGS__)
#define MDL_ITEM_275(ARRAY, NAME, ...)  NAME, MDL_ITEM_274(ARRAY, __VA_ARGS__)
#define MDL_ITEM_276(ARRAY, NAME, ...) NAME, MDL_ITEM_275(ARRAY, __VA_ARGS__)
#define MDL_ITEM_277(ARRAY, NAME, ...) NAME, MDL_ITEM_276(ARRAY, __VA_ARGS__)
#define MDL_ITEM_278(ARRAY, NAME, ...) NAME, MDL_ITEM_277(ARRAY, __VA_ARGS__)
#define MDL_ITEM_279(ARRAY, NAME, ...) NAME, MDL_ITEM_278(ARRAY, __VA_ARGS__)
#define MDL_ITEM_280(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_279(ARRAY, __VA_ARGS__)

#define MDL_ITEM_281(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_280(ARRAY, __VA_ARGS__)
#define MDL_ITEM_282(ARRAY, NAME, ...)  NAME, MDL_ITEM_281(ARRAY, __VA_ARGS__)
#define MDL_ITEM_283(ARRAY, NAME, ...) NAME, MDL_ITEM_282(ARRAY, __VA_ARGS__)
#define MDL_ITEM_284(ARRAY, NAME, ...) NAME, MDL_ITEM_283(ARRAY, __VA_ARGS__)
#define MDL_ITEM_285(ARRAY, NAME, ...) NAME, MDL_ITEM_284(ARRAY, __VA_ARGS__)
#define MDL_ITEM_286(ARRAY, NAME, ...) NAME, MDL_ITEM_285(ARRAY, __VA_ARGS__)
#define MDL_ITEM_287(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_286(ARRAY, __VA_ARGS__)

#define MDL_ITEM_288(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_287(ARRAY, __VA_ARGS__)
#define MDL_ITEM_289(ARRAY, NAME, ...)  NAME, MDL_ITEM_288(ARRAY, __VA_ARGS__)
#define MDL_ITEM_290(ARRAY, NAME, ...) NAME, MDL_ITEM_289(ARRAY, __VA_ARGS__)
#define MDL_ITEM_291(ARRAY, NAME, ...) NAME, MDL_ITEM_290(ARRAY, __VA_ARGS__)
#define MDL_ITEM_292(ARRAY, NAME, ...) NAME, MDL_ITEM_291(ARRAY, __VA_ARGS__)
#define MDL_ITEM_293(ARRAY, NAME, ...) NAME, MDL_ITEM_292(ARRAY, __VA_ARGS__)
#define MDL_ITEM_294(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_293(ARRAY, __VA_ARGS__)

#define MDL_ITEM_295(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_294(ARRAY, __VA_ARGS__)
#define MDL_ITEM_296(ARRAY, NAME, ...)  NAME, MDL_ITEM_295(ARRAY, __VA_ARGS__)
#define MDL_ITEM_297(ARRAY, NAME, ...) NAME, MDL_ITEM_296(ARRAY, __VA_ARGS__)
#define MDL_ITEM_298(ARRAY, NAME, ...) NAME, MDL_ITEM_297(ARRAY, __VA_ARGS__)
#define MDL_ITEM_299(ARRAY, NAME, ...) NAME, MDL_ITEM_298(ARRAY, __VA_ARGS__)
#define MDL_ITEM_300(ARRAY, NAME, ...) NAME, MDL_ITEM_299(ARRAY, __VA_ARGS__)
#define MDL_ITEM_301(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_300(ARRAY, __VA_ARGS__)

#define MDL_ITEM_302(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_301(ARRAY, __VA_ARGS__)
#define MDL_ITEM_303(ARRAY, NAME, ...)  NAME, MDL_ITEM_302(ARRAY, __VA_ARGS__)
#define MDL_ITEM_304(ARRAY, NAME, ...) NAME, MDL_ITEM_303(ARRAY, __VA_ARGS__)
#define MDL_ITEM_305(ARRAY, NAME, ...) NAME, MDL_ITEM_304(ARRAY, __VA_ARGS__)
#define MDL_ITEM_306(ARRAY, NAME, ...) NAME, MDL_ITEM_305(ARRAY, __VA_ARGS__)
#define MDL_ITEM_307(ARRAY, NAME, ...) NAME, MDL_ITEM_306(ARRAY, __VA_ARGS__)
#define MDL_ITEM_308(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_307(ARRAY, __VA_ARGS__)

#define MDL_ITEM_309(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_308(ARRAY, __VA_ARGS__)
#define MDL_ITEM_310(ARRAY, NAME, ...)  NAME, MDL_ITEM_309(ARRAY, __VA_ARGS__)
#define MDL_ITEM_311(ARRAY, NAME, ...) NAME, MDL_ITEM_310(ARRAY, __VA_ARGS__)
#define MDL_ITEM_312(ARRAY, NAME, ...) NAME, MDL_ITEM_311(ARRAY, __VA_ARGS__)
#define MDL_ITEM_313(ARRAY, NAME, ...) NAME, MDL_ITEM_312(ARRAY, __VA_ARGS__)
#define MDL_ITEM_314(ARRAY, NAME, ...) NAME, MDL_ITEM_313(ARRAY, __VA_ARGS__)
#define MDL_ITEM_315(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_314(ARRAY, __VA_ARGS__)

#define MDL_ITEM_316(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_315(ARRAY, __VA_ARGS__)
#define MDL_ITEM_317(ARRAY, NAME, ...)  NAME, MDL_ITEM_316(ARRAY, __VA_ARGS__)
#define MDL_ITEM_318(ARRAY, NAME, ...) NAME, MDL_ITEM_317(ARRAY, __VA_ARGS__)
#define MDL_ITEM_319(ARRAY, NAME, ...) NAME, MDL_ITEM_318(ARRAY, __VA_ARGS__)
#define MDL_ITEM_320(ARRAY, NAME, ...) NAME, MDL_ITEM_319(ARRAY, __VA_ARGS__)
#define MDL_ITEM_321(ARRAY, NAME, ...) NAME, MDL_ITEM_320(ARRAY, __VA_ARGS__)
#define MDL_ITEM_322(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_321(ARRAY, __VA_ARGS__)

#define MDL_ITEM_323(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_322(ARRAY, __VA_ARGS__)
#define MDL_ITEM_324(ARRAY, NAME, ...)  NAME, MDL_ITEM_323(ARRAY, __VA_ARGS__)
#define MDL_ITEM_325(ARRAY, NAME, ...) NAME, MDL_ITEM_324(ARRAY, __VA_ARGS__)
#define MDL_ITEM_326(ARRAY, NAME, ...) NAME, MDL_ITEM_325(ARRAY, __VA_ARGS__)
#define MDL_ITEM_327(ARRAY, NAME, ...) NAME, MDL_ITEM_326(ARRAY, __VA_ARGS__)
#define MDL_ITEM_328(ARRAY, NAME, ...) NAME, MDL_ITEM_327(ARRAY, __VA_ARGS__)
#define MDL_ITEM_329(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_328(ARRAY, __VA_ARGS__)

#define MDL_ITEM_330(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_329(ARRAY, __VA_ARGS__)
#define MDL_ITEM_331(ARRAY, NAME, ...)  NAME, MDL_ITEM_330(ARRAY, __VA_ARGS__)
#define MDL_ITEM_332(ARRAY, NAME, ...) NAME, MDL_ITEM_331(ARRAY, __VA_ARGS__)
#define MDL_ITEM_333(ARRAY, NAME, ...) NAME, MDL_ITEM_332(ARRAY, __VA_ARGS__)
#define MDL_ITEM_334(ARRAY, NAME, ...) NAME, MDL_ITEM_333(ARRAY, __VA_ARGS__)
#define MDL_ITEM_335(ARRAY, NAME, ...) NAME, MDL_ITEM_334(ARRAY, __VA_ARGS__)
#define MDL_ITEM_336(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_335(ARRAY, __VA_ARGS__)

#define MDL_ITEM_337(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_336(ARRAY, __VA_ARGS__)
#define MDL_ITEM_338(ARRAY, NAME, ...)  NAME, MDL_ITEM_337(ARRAY, __VA_ARGS__)
#define MDL_ITEM_339(ARRAY, NAME, ...) NAME, MDL_ITEM_338(ARRAY, __VA_ARGS__)
#define MDL_ITEM_340(ARRAY, NAME, ...) NAME, MDL_ITEM_339(ARRAY, __VA_ARGS__)
#define MDL_ITEM_341(ARRAY, NAME, ...) NAME, MDL_ITEM_340(ARRAY, __VA_ARGS__)
#define MDL_ITEM_342(ARRAY, NAME, ...) NAME, MDL_ITEM_341(ARRAY, __VA_ARGS__)
#define MDL_ITEM_343(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_342(ARRAY, __VA_ARGS__)

#define MDL_ITEM_344(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_343(ARRAY, __VA_ARGS__)
#define MDL_ITEM_345(ARRAY, NAME, ...)  NAME, MDL_ITEM_344(ARRAY, __VA_ARGS__)
#define MDL_ITEM_346(ARRAY, NAME, ...) NAME, MDL_ITEM_345(ARRAY, __VA_ARGS__)
#define MDL_ITEM_347(ARRAY, NAME, ...) NAME, MDL_ITEM_346(ARRAY, __VA_ARGS__)
#define MDL_ITEM_348(ARRAY, NAME, ...) NAME, MDL_ITEM_347(ARRAY, __VA_ARGS__)
#define MDL_ITEM_349(ARRAY, NAME, ...) NAME, MDL_ITEM_348(ARRAY, __VA_ARGS__)
#define MDL_ITEM_350(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_349(ARRAY, __VA_ARGS__)

#define MDL_ITEM_351(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_350(ARRAY, __VA_ARGS__)
#define MDL_ITEM_352(ARRAY, NAME, ...)  NAME, MDL_ITEM_351(ARRAY, __VA_ARGS__)
#define MDL_ITEM_353(ARRAY, NAME, ...) NAME, MDL_ITEM_352(ARRAY, __VA_ARGS__)
#define MDL_ITEM_354(ARRAY, NAME, ...) NAME, MDL_ITEM_353(ARRAY, __VA_ARGS__)
#define MDL_ITEM_355(ARRAY, NAME, ...) NAME, MDL_ITEM_354(ARRAY, __VA_ARGS__)
#define MDL_ITEM_356(ARRAY, NAME, ...) NAME, MDL_ITEM_355(ARRAY, __VA_ARGS__)
#define MDL_ITEM_357(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_356(ARRAY, __VA_ARGS__)

#define MDL_ITEM_358(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_357(ARRAY, __VA_ARGS__)
#define MDL_ITEM_359(ARRAY, NAME, ...)  NAME, MDL_ITEM_358(ARRAY, __VA_ARGS__)
#define MDL_ITEM_360(ARRAY, NAME, ...) NAME, MDL_ITEM_359(ARRAY, __VA_ARGS__)
#define MDL_ITEM_361(ARRAY, NAME, ...) NAME, MDL_ITEM_360(ARRAY, __VA_ARGS__)
#define MDL_ITEM_362(ARRAY, NAME, ...) NAME, MDL_ITEM_361(ARRAY, __VA_ARGS__)
#define MDL_ITEM_363(ARRAY, NAME, ...) NAME, MDL_ITEM_362(ARRAY, __VA_ARGS__)
#define MDL_ITEM_364(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_363(ARRAY, __VA_ARGS__)

#define MDL_ITEM_365(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_364(ARRAY, __VA_ARGS__)
#define MDL_ITEM_366(ARRAY, NAME, ...)  NAME, MDL_ITEM_365(ARRAY, __VA_ARGS__)
#define MDL_ITEM_367(ARRAY, NAME, ...) NAME, MDL_ITEM_366(ARRAY, __VA_ARGS__)
#define MDL_ITEM_368(ARRAY, NAME, ...) NAME, MDL_ITEM_367(ARRAY, __VA_ARGS__)
#define MDL_ITEM_369(ARRAY, NAME, ...) NAME, MDL_ITEM_368(ARRAY, __VA_ARGS__)
#define MDL_ITEM_370(ARRAY, NAME, ...) NAME, MDL_ITEM_369(ARRAY, __VA_ARGS__)
#define MDL_ITEM_371(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_370(ARRAY, __VA_ARGS__)

#define MDL_ITEM_372(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_371(ARRAY, __VA_ARGS__)
#define MDL_ITEM_373(ARRAY, NAME, ...)  NAME, MDL_ITEM_372(ARRAY, __VA_ARGS__)
#define MDL_ITEM_374(ARRAY, NAME, ...) NAME, MDL_ITEM_373(ARRAY, __VA_ARGS__)
#define MDL_ITEM_375(ARRAY, NAME, ...) NAME, MDL_ITEM_374(ARRAY, __VA_ARGS__)
#define MDL_ITEM_376(ARRAY, NAME, ...) NAME, MDL_ITEM_375(ARRAY, __VA_ARGS__)
#define MDL_ITEM_377(ARRAY, NAME, ...) NAME, MDL_ITEM_376(ARRAY, __VA_ARGS__)
#define MDL_ITEM_378(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_377(ARRAY, __VA_ARGS__)

#define MDL_ITEM_379(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_378(ARRAY, __VA_ARGS__)
#define MDL_ITEM_380(ARRAY, NAME, ...)  NAME, MDL_ITEM_379(ARRAY, __VA_ARGS__)
#define MDL_ITEM_381(ARRAY, NAME, ...) NAME, MDL_ITEM_380(ARRAY, __VA_ARGS__)
#define MDL_ITEM_382(ARRAY, NAME, ...) NAME, MDL_ITEM_381(ARRAY, __VA_ARGS__)
#define MDL_ITEM_383(ARRAY, NAME, ...) NAME, MDL_ITEM_382(ARRAY, __VA_ARGS__)
#define MDL_ITEM_384(ARRAY, NAME, ...) NAME, MDL_ITEM_383(ARRAY, __VA_ARGS__)
#define MDL_ITEM_385(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_384(ARRAY, __VA_ARGS__)

#define MDL_ITEM_386(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_385(ARRAY, __VA_ARGS__)
#define MDL_ITEM_387(ARRAY, NAME, ...)  NAME, MDL_ITEM_386(ARRAY, __VA_ARGS__)
#define MDL_ITEM_388(ARRAY, NAME, ...) NAME, MDL_ITEM_387(ARRAY, __VA_ARGS__)
#define MDL_ITEM_389(ARRAY, NAME, ...) NAME, MDL_ITEM_388(ARRAY, __VA_ARGS__)
#define MDL_ITEM_390(ARRAY, NAME, ...) NAME, MDL_ITEM_389(ARRAY, __VA_ARGS__)
#define MDL_ITEM_391(ARRAY, NAME, ...) NAME, MDL_ITEM_390(ARRAY, __VA_ARGS__)
#define MDL_ITEM_392(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_391(ARRAY, __VA_ARGS__)

#define MDL_ITEM_393(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_392(ARRAY, __VA_ARGS__)
#define MDL_ITEM_394(ARRAY, NAME, ...)  NAME, MDL_ITEM_393(ARRAY, __VA_ARGS__)
#define MDL_ITEM_395(ARRAY, NAME, ...) NAME, MDL_ITEM_394(ARRAY, __VA_ARGS__)
#define MDL_ITEM_396(ARRAY, NAME, ...) NAME, MDL_ITEM_395(ARRAY, __VA_ARGS__)
#define MDL_ITEM_397(ARRAY, NAME, ...) NAME, MDL_ITEM_396(ARRAY, __VA_ARGS__)
#define MDL_ITEM_398(ARRAY, NAME, ...) NAME, MDL_ITEM_397(ARRAY, __VA_ARGS__)
#define MDL_ITEM_399(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_398(ARRAY, __VA_ARGS__)

#define MDL_ITEM_400(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_399(ARRAY, __VA_ARGS__)
#define MDL_ITEM_401(ARRAY, NAME, ...)  NAME, MDL_ITEM_400(ARRAY, __VA_ARGS__)
#define MDL_ITEM_402(ARRAY, NAME, ...) NAME, MDL_ITEM_401(ARRAY, __VA_ARGS__)
#define MDL_ITEM_403(ARRAY, NAME, ...) NAME, MDL_ITEM_402(ARRAY, __VA_ARGS__)
#define MDL_ITEM_404(ARRAY, NAME, ...) NAME, MDL_ITEM_403(ARRAY, __VA_ARGS__)
#define MDL_ITEM_405(ARRAY, NAME, ...) NAME, MDL_ITEM_404(ARRAY, __VA_ARGS__)
#define MDL_ITEM_406(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_405(ARRAY, __VA_ARGS__)

#define MDL_ITEM_407(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_406(ARRAY, __VA_ARGS__)
#define MDL_ITEM_408(ARRAY, NAME, ...)  NAME, MDL_ITEM_407(ARRAY, __VA_ARGS__)
#define MDL_ITEM_409(ARRAY, NAME, ...) NAME, MDL_ITEM_408(ARRAY, __VA_ARGS__)
#define MDL_ITEM_410(ARRAY, NAME, ...) NAME, MDL_ITEM_409(ARRAY, __VA_ARGS__)
#define MDL_ITEM_411(ARRAY, NAME, ...) NAME, MDL_ITEM_410(ARRAY, __VA_ARGS__)
#define MDL_ITEM_412(ARRAY, NAME, ...) NAME, MDL_ITEM_411(ARRAY, __VA_ARGS__)
#define MDL_ITEM_413(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_412(ARRAY, __VA_ARGS__)

#define MDL_ITEM_414(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_413(ARRAY, __VA_ARGS__)
#define MDL_ITEM_415(ARRAY, NAME, ...)  NAME, MDL_ITEM_414(ARRAY, __VA_ARGS__)
#define MDL_ITEM_416(ARRAY, NAME, ...) NAME, MDL_ITEM_415(ARRAY, __VA_ARGS__)
#define MDL_ITEM_417(ARRAY, NAME, ...) NAME, MDL_ITEM_416(ARRAY, __VA_ARGS__)
#define MDL_ITEM_418(ARRAY, NAME, ...) NAME, MDL_ITEM_417(ARRAY, __VA_ARGS__)
#define MDL_ITEM_419(ARRAY, NAME, ...) NAME, MDL_ITEM_418(ARRAY, __VA_ARGS__)
#define MDL_ITEM_420(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_419(ARRAY, __VA_ARGS__)

#define MDL_ITEM_421(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_420(ARRAY, __VA_ARGS__)
#define MDL_ITEM_422(ARRAY, NAME, ...)  NAME, MDL_ITEM_421(ARRAY, __VA_ARGS__)
#define MDL_ITEM_423(ARRAY, NAME, ...) NAME, MDL_ITEM_422(ARRAY, __VA_ARGS__)
#define MDL_ITEM_424(ARRAY, NAME, ...) NAME, MDL_ITEM_423(ARRAY, __VA_ARGS__)
#define MDL_ITEM_425(ARRAY, NAME, ...) NAME, MDL_ITEM_424(ARRAY, __VA_ARGS__)
#define MDL_ITEM_426(ARRAY, NAME, ...) NAME, MDL_ITEM_425(ARRAY, __VA_ARGS__)
#define MDL_ITEM_427(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_426(ARRAY, __VA_ARGS__)

#define MDL_ITEM_428(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_427(ARRAY, __VA_ARGS__)
#define MDL_ITEM_429(ARRAY, NAME, ...)  NAME, MDL_ITEM_428(ARRAY, __VA_ARGS__)
#define MDL_ITEM_430(ARRAY, NAME, ...) NAME, MDL_ITEM_429(ARRAY, __VA_ARGS__)
#define MDL_ITEM_431(ARRAY, NAME, ...) NAME, MDL_ITEM_430(ARRAY, __VA_ARGS__)
#define MDL_ITEM_432(ARRAY, NAME, ...) NAME, MDL_ITEM_431(ARRAY, __VA_ARGS__)
#define MDL_ITEM_433(ARRAY, NAME, ...) NAME, MDL_ITEM_432(ARRAY, __VA_ARGS__)
#define MDL_ITEM_434(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_433(ARRAY, __VA_ARGS__)

#define MDL_ITEM_435(ARRAY, NAME, ...)  NAME, NULL, model_param_clear)); MDL_ITEM_434(ARRAY, __VA_ARGS__)
#define MDL_ITEM_436(ARRAY, NAME, ...)  NAME, MDL_ITEM_435(ARRAY, __VA_ARGS__)
#define MDL_ITEM_437(ARRAY, NAME, ...) NAME, MDL_ITEM_436(ARRAY, __VA_ARGS__)
#define MDL_ITEM_438(ARRAY, NAME, ...) NAME, MDL_ITEM_437(ARRAY, __VA_ARGS__)
#define MDL_ITEM_439(ARRAY, NAME, ...) NAME, MDL_ITEM_438(ARRAY, __VA_ARGS__)
#define MDL_ITEM_440(ARRAY, NAME, ...) NAME, MDL_ITEM_439(ARRAY, __VA_ARGS__)
#define MDL_ITEM_441(ARRAY, NAME, ...) array_push_back(ARRAY, array_create_pointer(&(mfield_t)NAME, MDL_ITEM_440(ARRAY, __VA_ARGS__)

#define mparams_fill_array(ARRAY, ...) MDL_ITEM(ARRAY, MDL_VA_NARGS(__VA_ARGS__), __VA_ARGS__)
#define mparams_create_array(ARRAY, ...) array_t* ARRAY = array_create();mparams_fill_array(ARRAY, __VA_ARGS__)


typedef enum {
    MODEL_BOOL = 0,

    MODEL_SMALLINT,
    MODEL_INT,
    MODEL_BIGINT,
    MODEL_FLOAT,
    MODEL_DOUBLE,
    MODEL_DECIMAL,
    MODEL_MONEY,

    MODEL_DATE,
    MODEL_TIME,
    MODEL_TIMETZ,
    MODEL_TIMESTAMP,
    MODEL_TIMESTAMPTZ,

    MODEL_JSON,

    MODEL_BINARY,
    MODEL_VARCHAR,
    MODEL_CHAR,
    MODEL_TEXT,
    MODEL_ENUM,
    MODEL_ARRAY,
} mtype_e;

typedef struct {
    union {
        short _short;
        int _int;
        long long _bigint;
        float _float;
        double _double;
        long double _ldouble;
        tm_t* _tm;
        jsondoc_t* _jsondoc;
        enums_t* _enum;
        array_t* _array;
    };
    str_t* _string;
} mvalue_t;

typedef struct mfield {
    const mtype_e type;
    unsigned dirty : 1;
    char name[64];
    mvalue_t value;
    mvalue_t oldvalue;
} mfield_t;

typedef struct model {
    mfield_t*(*first_field)(void* arg);
    int(*fields_count)(void* arg);
    const char*(*table)(void* arg);
    const char**(*primary_key)(void* arg);
    int(*primary_key_count)(void* arg);
} model_t;

typedef struct modelview {
    mfield_t*(*first_field)(void* arg);
    int(*fields_count)(void* arg);
} modelview_t;

void* model_get(const char* dbid, void*(create_instance)(void), array_t* params);
int model_create(const char* dbid, void* arg);
int model_update(const char* dbid, void* arg);
int model_delete(const char* dbid, void* arg);
void* model_one(const char* dbid, void*(create_instance)(void), const char* format, array_t* params);
array_t* model_list(const char* dbid, void*(create_instance)(void), const char* format, array_t* params);
int model_execute(const char* dbid, const char* format, array_t* params);

jsontok_t* model_to_json(void* arg, jsondoc_t* document);
char* model_stringify(void* arg);
char* model_list_stringify(array_t* array);
void model_free(void* arg);




short model_bool(mfield_t* field);
short model_smallint(mfield_t* field);
int model_int(mfield_t* field);
long long int model_bigint(mfield_t* field);
float model_float(mfield_t* field);
double model_double(mfield_t* field);
long double model_decimal(mfield_t* field);
double model_money(mfield_t* field);

tm_t* model_timestamp(mfield_t* field);
tm_t* model_timestamptz(mfield_t* field);
tm_t* model_date(mfield_t* field);
tm_t* model_time(mfield_t* field);
tm_t* model_timetz(mfield_t* field);

jsondoc_t* model_json(mfield_t* field);

str_t* model_binary(mfield_t* field);
str_t* model_varchar(mfield_t* field);
str_t* model_char(mfield_t* field);
str_t* model_text(mfield_t* field);
str_t* model_enum(mfield_t* field);

array_t* model_array(mfield_t* field);




int model_set_bool(mfield_t* field, short value);
int model_set_smallint(mfield_t* field, short value);
int model_set_int(mfield_t* field, int value);
int model_set_bigint(mfield_t* field, long long int value);
int model_set_float(mfield_t* field, float value);
int model_set_double(mfield_t* field, double value);
int model_set_decimal(mfield_t* field, long double value);
int model_set_money(mfield_t* field, double value);

int model_set_timestamp(mfield_t* field, tm_t* value);
int model_set_timestamptz(mfield_t* field, tm_t* value);
int model_set_date(mfield_t* field, tm_t* value);
int model_set_time(mfield_t* field, tm_t* value);
int model_set_timetz(mfield_t* field, tm_t* value);

int model_set_json(mfield_t* field, jsondoc_t* value);

int model_set_binary(mfield_t* field, const char* value, const size_t size);
int model_set_varchar(mfield_t* field, const char* value);
int model_set_char(mfield_t* field, const char* value);
int model_set_text(mfield_t* field, const char* value);
int model_set_enum(mfield_t* field, const char* value);

int model_set_array(mfield_t* field, array_t* value);



int model_set_bool_from_str(mfield_t* field, const char* value);
int model_set_smallint_from_str(mfield_t* field, const char* value);
int model_set_int_from_str(mfield_t* field, const char* value);
int model_set_bigint_from_str(mfield_t* field, const char* value);
int model_set_float_from_str(mfield_t* field, const char* value);
int model_set_double_from_str(mfield_t* field, const char* value);
int model_set_decimal_from_str(mfield_t* field, const char* value);
int model_set_money_from_str(mfield_t* field, const char* value);

int model_set_timestamp_from_str(mfield_t* field, const char* value);
int model_set_timestamptz_from_str(mfield_t* field, const char* value);
int model_set_date_from_str(mfield_t* field, const char* value);
int model_set_time_from_str(mfield_t* field, const char* value);
int model_set_timetz_from_str(mfield_t* field, const char* value);

int model_set_json_from_str(mfield_t* field, const char* value);

int model_set_binary_from_str(mfield_t* field, const char* value, size_t size);
int model_set_varchar_from_str(mfield_t* field, const char* value, size_t size);
int model_set_char_from_str(mfield_t* field, const char* value, size_t size);
int model_set_text_from_str(mfield_t* field, const char* value, size_t size);
int model_set_enum_from_str(mfield_t* field, const char* value, size_t size);

int model_set_array_from_str(mfield_t* field, const char* value);




str_t* model_bool_to_str(mfield_t* field);
str_t* model_smallint_to_str(mfield_t* field);
str_t* model_int_to_str(mfield_t* field);
str_t* model_bigint_to_str(mfield_t* field);
str_t* model_float_to_str(mfield_t* field);
str_t* model_double_to_str(mfield_t* field);
str_t* model_decimal_to_str(mfield_t* field);
str_t* model_money_to_str(mfield_t* field);

str_t* model_timestamp_to_str(mfield_t* field);
str_t* model_timestamptz_to_str(mfield_t* field);
str_t* model_date_to_str(mfield_t* field);
str_t* model_time_to_str(mfield_t* field);
str_t* model_timetz_to_str(mfield_t* field);

str_t* model_json_to_str(mfield_t* field);
str_t* model_array_to_str(mfield_t* field);

str_t* model_field_to_string(mfield_t* field);

void model_param_clear(void* field);
void model_params_clear(void* params, const size_t size);
void model_params_free(void* params, const size_t size);

#endif
