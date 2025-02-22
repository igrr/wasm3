//
//  m3_bind.c
//  m3
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_exec.h"
#include "m3_env.h"
#include "m3_exception.h"


typedef struct M3State
{
    pc_t        pc;
    m3stack_t   sp;
    u8 *        mem;
}
M3State;


// TODO: This binding code only work'n for System V AMD64 ABI calling convention (macOS & Linux)
// Needs work for MS cdecl

#define d_m3BindingArgList i64 _i0, i64 _i1, i64 _i2, i64 _i3, i64 _i4, i64 _i5, \
                           f64 _f0, f64 _f1, f64 _f2, f64 _f3, f64 _f4
#define d_m3BindingArgs _i0, _i1, _i2, _i3, _i4, _i5, \
                        _f0, _f1, _f2, _f3, _f4
#define d_m3BindingDefaultArgs 0,0,0,0,0,0, \
                               0.,0.,0.,0.,0.

typedef m3ret_t (* M3ArgPusher) (d_m3BindingArgList, M3State * i_state);
typedef f64 (* M3ArgPusherFpReturn) (d_m3BindingArgList, M3State * i_state);


m3ret_t PushArg_runtime (d_m3BindingArgList, M3State * _state)
{
    M3MemoryHeader * info = (M3MemoryHeader *) _state->mem - 1;
    _i0 = (i64) info->runtime;
    M3ArgPusher pusher = (M3ArgPusher)(* _state->pc++);
    return pusher (d_m3BindingArgs, _state);
}


#define d_argPusherPointer(INDEX)                                       \
m3ret_t PushArg_p##INDEX (d_m3BindingArgList, M3State * _state)         \
{                                                                       \
    i32 offset = (u32) * (_state->sp++);                                \
    _i##INDEX = (i64) (_state->mem + offset);                           \
    M3ArgPusher pusher = (M3ArgPusher)(* _state->pc++);                 \
    return pusher (d_m3BindingArgs, _state);                            \
}
//printf ("push ptr: r%d off: %d\n", INDEX, offset);

//printf ("push [%d]: %" PRId64 "\n", INDEX, _i##INDEX);
#define d_argPusherInt(INDEX)                                           \
m3ret_t PushArg_i##INDEX (d_m3BindingArgList, M3State * _state)         \
{                                                                       \
    _i##INDEX = * (_state->sp++);                                       \
    M3ArgPusher pusher = (M3ArgPusher)(* _state->pc++);                 \
    return pusher (d_m3BindingArgs, _state);                            \
}

//printf ("push [%d]: %lf\n", INDEX, * (TYPE *) (_state->sp));

#define d_argPusherFloat(INDEX,TYPE)                                    \
m3ret_t PushArg_##TYPE##_##INDEX (d_m3BindingArgList, M3State * _state) \
{                                                                       \
    _f##INDEX = * (TYPE *) (_state->sp++);                              \
    M3ArgPusher pusher = (M3ArgPusher)(* _state->pc++);                 \
    return pusher (d_m3BindingArgs, _state);                            \
}




d_argPusherPointer  (0)         d_argPusherPointer  (1)         d_argPusherPointer  (2)         d_argPusherPointer  (3)
d_argPusherInt      (0)         d_argPusherInt      (1)         d_argPusherInt      (2)         d_argPusherInt      (3)
d_argPusherFloat    (0, f32)    d_argPusherFloat    (1, f32)    d_argPusherFloat    (2, f32)    d_argPusherFloat    (3, f32)
d_argPusherFloat    (0, f64)    d_argPusherFloat    (1, f64)    d_argPusherFloat    (2, f64)    d_argPusherFloat    (3, f64)

d_argPusherPointer  (4)         d_argPusherPointer  (5)
d_argPusherInt      (4)         d_argPusherInt      (5)
d_argPusherFloat    (4, f32)
d_argPusherFloat    (4, f64)

M3ArgPusher c_m3PointerPushers  [] = { PushArg_p0, PushArg_p1, PushArg_p2, PushArg_p3, PushArg_p4, PushArg_p5, NULL };      // one dummy is required
M3ArgPusher c_m3IntPushers      [] = { PushArg_i0, PushArg_i1, PushArg_i2, PushArg_i3, PushArg_i4, PushArg_i5, NULL };
M3ArgPusher c_m3Float32Pushers  [] = { PushArg_f32_0, PushArg_f32_1, PushArg_f32_2, PushArg_f32_3, PushArg_f32_4, NULL };
M3ArgPusher c_m3Float64Pushers  [] = { PushArg_f64_0, PushArg_f64_1, PushArg_f64_2, PushArg_f64_3, PushArg_f64_4, NULL };



d_m3RetSig  CallTrappingCFunction_void  (d_m3OpSig)
{
    M3ArgPusher pusher = (M3ArgPusher) (* _pc++);
    M3State state = { _pc, _sp, _mem };

    m3ret_t r = (m3ret_t) pusher (d_m3BindingDefaultArgs, & state);

    return r;
}


d_m3RetSig  CallCFunction_i64  (d_m3OpSig)
{
    M3ArgPusher pusher = (M3ArgPusher) (* _pc++);
    M3State state = { _pc, _sp, _mem };

    i64 r = (i64) pusher (d_m3BindingDefaultArgs, & state);
    * _sp = r;

    return 0;
}


d_m3RetSig  CallCFunction_f64  (d_m3OpSig)
{
    M3ArgPusherFpReturn pusher = (M3ArgPusherFpReturn) (* _pc++);
    M3State state = { _pc, _sp, _mem };

    f64 r = (f64) pusher (d_m3BindingDefaultArgs, & state);
    * (f64 *) (_sp) = r;

    return 0;
}


d_m3RetSig  CallCFunction_f32  (d_m3OpSig)
{
    M3ArgPusherFpReturn pusher = (M3ArgPusherFpReturn) (* _pc++);
    M3State state = { _pc, _sp, _mem };

    f32 r = (f32) pusher (d_m3BindingDefaultArgs, & state);
    * (f32 *) (_sp) = r;

    return 0;
}


d_m3RetSig  CallCFunction_ptr  (d_m3OpSig)
{
    M3ArgPusher pusher = (M3ArgPusher) (* _pc++);
    M3State state = { _pc, _sp, _mem };

    const u8 * r = (const u8*) pusher (d_m3BindingDefaultArgs, & state);

    void ** ptr = (void **) _mem;
    IM3Runtime runtime = (IM3Runtime) (* (ptr - 2));

    size_t offset = r - (const u8 *) runtime->memory.wasmPages;

    * (i32 *) (_sp) = (i32) offset;

    return 0;
}



u8  ConvertTypeCharToTypeId (char i_code)
{
    u8 type = 0;

//  if (i_code > c_m3Type_ptr)
    {
        if      (i_code == 'v') type = c_m3Type_void;
        else if (i_code == '*') type = c_m3Type_ptr;
        else if (i_code == 'T') type = c_m3Type_trap;
        else if (i_code == '8') type = c_m3Type_i32;
        else if (i_code == 'f') type = c_m3Type_f32;
        else if (i_code == 'F') type = c_m3Type_f64;
        else if (i_code == 'i') type = c_m3Type_i32;
        else if (i_code == 'I') type = c_m3Type_i64;
        else if (i_code == 'R') type = c_m3Type_runtime;
    }

    return type;
}


M3Result  ValidateSignature  (IM3Function i_function, bool * o_traps, u8 * o_normalizedSignature, ccstr_t i_linkingSignature)
{
    M3Result result = c_m3Err_none;

    * o_traps = false;

    cstr_t sig = i_linkingSignature;

    bool hasReturn = false;
    u32 numArgs = 0;

    // check for trap flag
    u8 type = ConvertTypeCharToTypeId (* sig);
    if (type == c_m3Type_trap)
    {
        * o_traps = true;
        ++sig;
    }

    bool parsingArgs = false;
    while (* sig)
    {
        if (numArgs >= c_m3MaxNumFunctionArgs)
            _throw ("arg count overflow");

        char typeChar = * sig++;

        if (typeChar == '(')
        {
            if (not hasReturn)
                _throw ("malformed function signature; missing return type");

            parsingArgs = true;
            continue;
        }
        else if ( typeChar == ' ')
            continue;
        else if (typeChar == ')')
            break;

        type = ConvertTypeCharToTypeId (typeChar);

        if (type)
        {
            * o_normalizedSignature++ = type;

            if (type == c_m3Type_trap)
                _throw ("malformed function signature");

            if (not parsingArgs)
            {
                if (hasReturn)
                    _throw ("malformed function signature; too many return types");

                hasReturn = true;
            }
            else
            {
                if (type != c_m3Type_runtime)
                    ++numArgs;
            }
        }
        else _throw ("unknown argument type char");
    }

    if (GetFunctionNumArgs (i_function) != numArgs)
        _throw ("function arg count mismatch");

    _catch: return result;
}


M3Result  m3_RegisterFunction  (IM3Runtime io_runtime,  const char * const i_functionName,  const char * const i_signature,  const void * const i_function /* , const void * const i_ref */)
{
    M3Result result = c_m3Err_none;


    return result;
}


typedef M3Result  (* M3Linker)  (IM3Module io_module,  IM3Function io_function,  const char * const i_signature,  const void * i_function);



M3Result  LinkCFunction  (IM3Module io_module,  IM3Function io_function,  const char * const i_signature,  const void * i_function)
{
    M3Result result = c_m3Err_none;

    M3ArgPusher pushers [c_m3MaxNumFunctionArgs + 1];
    u8 signature [1 /* return */ + c_m3MaxNumFunctionArgs + 2];

    M3_INIT(pushers);
    M3_INIT(signature);

    bool trappingFunction = false;

    result = ValidateSignature (io_function, & trappingFunction, signature, i_signature);

    if (not result)
    {
        u8 returnType = signature [0];
        u8 * sig = & signature [1];

        u32 intIndex = 0;
        u32 floatIndex = 0;

        u32 i = 0;
        while (* sig)
        {
            M3ArgPusher * pusher = & pushers [i];
            u8 type = * sig;

            if      (type == c_m3Type_ptr)      * pusher = c_m3PointerPushers   [intIndex++];
            else if (IsIntType (type))          * pusher = c_m3IntPushers       [intIndex++];
            else if (type == c_m3Type_f32)      * pusher = c_m3Float32Pushers   [floatIndex++];
            else if (type == c_m3Type_f64)      * pusher = c_m3Float64Pushers   [floatIndex++];
            else if (type == c_m3Type_runtime)
            {
                * pusher = PushArg_runtime;
                d_m3Assert (i == 0); // can only push to arg0
                ++intIndex;
            }
            else
            {
                result = "FIX";
                m3NotImplemented();
            }

            if (* pusher == NULL)
                _throw ("too many arguments in C binding");

            ++i; ++sig;
        }

        if (not result)
        {
            IM3CodePage page = AcquireCodePageWithCapacity (io_module->runtime, /*setup-func:*/ 1 + /*arg pushers:*/ i + /*target c-function:*/ 1);

            if (page)
            {
                io_function->compiled = GetPagePC (page);
                io_function->module = io_module;

                IM3Operation callerOp;

                if (trappingFunction)
                {
                    // TODO: returned types not implemented!
                    d_m3Assert (returnType == c_m3Type_void);

                    callerOp = CallTrappingCFunction_void;
                }
                else
                {
                    callerOp = CallCFunction_i64;

                    if      (returnType == c_m3Type_f64)
                        callerOp = CallCFunction_f64;
                    else if (returnType == c_m3Type_f32)
                        callerOp = CallCFunction_f32;
                    else if (returnType == c_m3Type_ptr)
                        callerOp = CallCFunction_ptr;
                }

                EmitWord (page, callerOp);

                for (u32 j = 0; j < i; ++j)
                    EmitWord (page, pushers [j]);

                EmitWord (page, i_function);

                ReleaseCodePage (io_module->runtime, page);
            }
            else result = c_m3Err_mallocFailedCodePage;
        }
    }

	_catch:
	return result;
}


M3Result  FindAndLinkFunction      (IM3Module       io_module,
                                    ccstr_t         i_moduleName,
                                    ccstr_t         i_functionName,
                                    ccstr_t         i_signature,
                                    voidptr_t       i_function,
                                    const M3Linker  i_linker)
{
    M3Result result = c_m3Err_functionLookupFailed;
    
    for (u32 i = 0; i < io_module->numFunctions; ++i)
    {
        IM3Function f = & io_module->functions [i];
        
        if (f->import.moduleUtf8 and f->import.fieldUtf8)
        {
            if (strcmp (f->import.fieldUtf8, i_functionName) == 0 and
                strcmp (f->import.moduleUtf8, i_moduleName) == 0)
            {
                result = i_linker (io_module, f, i_signature, i_function);
                if (result) return result;
            }
        }
    }

    return result;
}



M3Result  m3_LinkCFunction  (IM3Module io_module,
                            const char * const i_moduleName,
                            const char * const i_functionName,
                            const char * const i_signature,
                            const void * i_function)
{
    return FindAndLinkFunction (io_module, i_moduleName, i_functionName, i_signature, i_function, LinkCFunction);
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// TODO: validate signature as well?
M3Result  LinkRawFunction  (IM3Module io_module,  IM3Function io_function, ccstr_t signature,  const void * i_function)
{
    M3Result result = c_m3Err_none;                                                 d_m3Assert (io_module->runtime);
    
    IM3CodePage page = AcquireCodePageWithCapacity (io_module->runtime, 2);
    
    if (page)
    {
        io_function->compiled = GetPagePC (page);
        io_function->module = io_module;

        EmitWord (page, op_CallRawFunction);
        EmitWord (page, i_function);
        
        ReleaseCodePage (io_module->runtime, page);
    }
    else result = c_m3Err_mallocFailedCodePage;

    return result;
}


M3Result  m3_LinkRawFunction  (IM3Module            io_module,
                              const char * const    i_moduleName,
                              const char * const    i_functionName,
                              M3RawCall             i_function)
{
    return FindAndLinkFunction (io_module, i_moduleName, i_functionName, NULL, i_function, LinkRawFunction);
}



