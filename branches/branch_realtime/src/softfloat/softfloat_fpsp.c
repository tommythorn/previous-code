
/*============================================================================

 This C source file is an extension to the SoftFloat IEC/IEEE Floating-point
 Arithmetic Package, Release 2a.

 Written by Andreas Grabher for Previous, NeXT Computer Emulator.
 
=============================================================================*/

#include "softfloat.h"
#include "softfloat_fpsp_tables.h"


/*----------------------------------------------------------------------------
| Algorithms for transcendental functions supported by MC68881 and MC68882 
| mathematical coprocessors. The functions are derived from FPSP library.
*----------------------------------------------------------------------------*/

#define pi_sig      LIT64(0xc90fdaa22168c235)
#define pi_sig0     LIT64(0xc90fdaa22168c234)
#define pi_sig1     LIT64(0xc4c6628b80dc1cd1)

#define pi_exp      0x4000
#define piby2_exp   0x3FFF
#define piby4_exp   0x3FFE

#define one_exp     0x3FFF
#define one_sig     LIT64(0x8000000000000000)


/*----------------------------------------------------------------------------
 | Function for compactifying extended double-precision floating point values.
 *----------------------------------------------------------------------------*/

int32 floatx80_make_compact(int32 aExp, bits64 aSig)
{
    return (aExp<<16)|(aSig>>48);
}


/*----------------------------------------------------------------------------
 | Arc cosine
 *----------------------------------------------------------------------------*/

floatx80 floatx80_acos(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact;
    floatx80 fp0, fp1, one;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF && (bits64) (aSig<<1)) {
        return propagateFloatx80NaNOneArg(a);
    }
    if (aExp == 0 && aSig == 0) {
        float_raise(float_flag_inexact);
        return roundAndPackFloatx80(floatx80_rounding_precision, 0, piby2_exp, pi_sig, 0);
    }

    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact >= 0x3FFF8000) { // |X| >= 1
        if (aExp == one_exp && aSig == one_sig) { // |X| == 1
            if (aSign) { // X == -1
                a = packFloatx80(0, pi_exp, pi_sig);
                float_raise(float_flag_inexact);
                return floatx80_move(a);
            } else { // X == +1
                return packFloatx80(0, 0, 0);
            }
        } else { // |X| > 1
            float_raise(float_flag_invalid);
            a.low = floatx80_default_nan_low;
            a.high = floatx80_default_nan_high;
            return a;
        }
    } // |X| < 1
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    one = packFloatx80(0, one_exp, one_sig);
    fp0 = a;
    
    fp1 = floatx80_add(one, fp0);   // 1 + X
    fp0 = floatx80_sub(one, fp0);   // 1 - X
    fp0 = floatx80_div(fp0, fp1);   // (1-X)/(1+X)
    fp0 = floatx80_sqrt(fp0);       // SQRT((1-X)/(1+X))
    fp0 = floatx80_atan(fp0);       // ATAN(SQRT((1-X)/(1+X)))
    
    float_rounding_mode = user_rnd_mode;
    floatx80_rounding_precision = user_rnd_prec;
    
    a = floatx80_add(fp0, fp0);     // 2 * ATAN(SQRT((1-X)/(1+X)))
    
    float_raise(float_flag_inexact);
    
    return a;
}

/*----------------------------------------------------------------------------
 | Arc sine
 *----------------------------------------------------------------------------*/

floatx80 floatx80_asin(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact;
    floatx80 fp0, fp1, fp2, one;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF && (bits64) (aSig<<1)) {
        return propagateFloatx80NaNOneArg(a);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }

    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact >= 0x3FFF8000) { // |X| >= 1
        if (aExp == one_exp && aSig == one_sig) { // |X| == 1
            float_raise(float_flag_inexact);
            a = packFloatx80(aSign, piby2_exp, pi_sig);
            return floatx80_move(a);
        } else { // |X| > 1
            float_raise(float_flag_invalid);
            a.low = floatx80_default_nan_low;
            a.high = floatx80_default_nan_high;
            return a;
        }

    } // |X| < 1
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    one = packFloatx80(0, one_exp, one_sig);
    fp0 = a;

    fp1 = floatx80_sub(one, fp0);   // 1 - X
    fp2 = floatx80_add(one, fp0);   // 1 + X
    fp1 = floatx80_mul(fp2, fp1);   // (1+X)*(1-X)
    fp1 = floatx80_sqrt(fp1);       // SQRT((1+X)*(1-X))
    fp0 = floatx80_div(fp0, fp1);   // X/SQRT((1+X)*(1-X))
    
    float_rounding_mode = user_rnd_mode;
    floatx80_rounding_precision = user_rnd_prec;
    
    a = floatx80_atan(fp0);         // ATAN(X/SQRT((1+X)*(1-X)))
    
    float_raise(float_flag_inexact);

    return a;
}

/*----------------------------------------------------------------------------
 | Arc tangent
 *----------------------------------------------------------------------------*/

floatx80 floatx80_atan(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, tbl_index;
    floatx80 fp0, fp1, fp2, fp3, xsave;

    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        a = packFloatx80(aSign, piby2_exp, pi_sig);
        float_raise(float_flag_inexact);
        return floatx80_move(a);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    compact = floatx80_make_compact(aExp, aSig);
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;

    if (compact < 0x3FFB8000 || compact > 0x4002FFFF) { // |X| >= 16 or |X| < 1/16
        if (compact > 0x3FFF8000) { // |X| >= 16
            if (compact > 0x40638000) { // |X| > 2^(100)
                fp0 = packFloatx80(aSign, piby2_exp, pi_sig);
                fp1 = packFloatx80(aSign, 0x0001, one_sig);
                
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                a = floatx80_sub(fp0, fp1);
                
                float_raise(float_flag_inexact);
                
                return a;
            } else {
                fp0 = a;
                fp1 = packFloatx80(1, one_exp, one_sig); // -1
                fp1 = floatx80_div(fp1, fp0); // X' = -1/X
                xsave = fp1;
                fp0 = floatx80_mul(fp1, fp1); // Y = X'*X'
                fp1 = floatx80_mul(fp0, fp0); // Z = Y*Y
                fp3 = float64_to_floatx80(LIT64(0xBFB70BF398539E6A)); // C5
                fp2 = float64_to_floatx80(LIT64(0x3FBC7187962D1D7D)); // C4
                fp3 = floatx80_mul(fp3, fp1); // Z*C5
                fp2 = floatx80_mul(fp2, fp1); // Z*C4
                fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBFC24924827107B8))); // C3+Z*C5
                fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FC999999996263E))); // C2+Z*C4
                fp1 = floatx80_mul(fp1, fp3); // Z*(C3+Z*C5)
                fp2 = floatx80_mul(fp2, fp0); // Y*(C2+Z*C4)
                fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0xBFD5555555555536))); // C1+Z*(C3+Z*C5)
                fp0 = floatx80_mul(fp0, xsave); // X'*Y
                fp1 = floatx80_add(fp1, fp2); // [Y*(C2+Z*C4)]+[C1+Z*(C3+Z*C5)]
                fp0 = floatx80_mul(fp0, fp1); // X'*Y*([B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))]) ??
                fp0 = floatx80_add(fp0, xsave);
                fp1 = packFloatx80(aSign, piby2_exp, pi_sig);
                
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;

                a = floatx80_add(fp0, fp1);
                
                float_raise(float_flag_inexact);
                
                return a;
            }
        } else { // |X| < 1/16
            if (compact < 0x3FD78000) { // |X| < 2^(-40)
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                a = floatx80_move(a);
                
                float_raise(float_flag_inexact);
                
                return a;
            } else {
                fp0 = a;
                xsave = a;
                fp0 = floatx80_mul(fp0, fp0); // Y = X*X
                fp1 = floatx80_mul(fp0, fp0); // Z = Y*Y
                fp2 = float64_to_floatx80(LIT64(0x3FB344447F876989)); // B6
                fp3 = float64_to_floatx80(LIT64(0xBFB744EE7FAF45DB)); // B5
                fp2 = floatx80_mul(fp2, fp1); // Z*B6
                fp3 = floatx80_mul(fp3, fp1); // Z*B5
                fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FBC71C646940220))); // B4+Z*B6
                fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBFC24924921872F9))); // B3+Z*B5
                fp2 = floatx80_mul(fp2, fp1); // Z*(B4+Z*B6)
                fp1 = floatx80_mul(fp1, fp3); // Z*(B3+Z*B5)
                fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FC9999999998FA9))); // B2+Z*(B4+Z*B6)
                fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0xBFD5555555555555))); // B1+Z*(B3+Z*B5)
                fp2 = floatx80_mul(fp2, fp0); // Y*(B2+Z*(B4+Z*B6))
                fp0 = floatx80_mul(fp0, xsave); // X*Y
                fp1 = floatx80_add(fp1, fp2); // [B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))]
                fp0 = floatx80_mul(fp0, fp1); // X*Y*([B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))])
                
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                a = floatx80_add(fp0, xsave);
                
                float_raise(float_flag_inexact);
                
                return a;
            }
        }
    } else {
        aSig &= LIT64(0xF800000000000000);
        aSig |= LIT64(0x0400000000000000);
        xsave = packFloatx80(aSign, aExp, aSig); // F
        fp0 = a;
        fp1 = a; // X
        fp2 = packFloatx80(0, one_exp, one_sig); // 1
        fp1 = floatx80_mul(fp1, xsave); // X*F
        fp0 = floatx80_sub(fp0, xsave); // X-F
        fp1 = floatx80_add(fp1, fp2); // 1 + X*F
        fp0 = floatx80_div(fp0, fp1); // U = (X-F)/(1+X*F)
        
        tbl_index = compact;
        
        tbl_index &= 0x7FFF0000;
        tbl_index -= 0x3FFB0000;
        tbl_index >>= 1;
        tbl_index += compact&0x00007800;
        tbl_index >>= 11;
        
        fp3 = atan_tbl[tbl_index];
        
        fp3.high |= aSign ? 0x8000 : 0; // ATAN(F)
        
        fp1 = floatx80_mul(fp0, fp0); // V = U*U
        fp2 = float64_to_floatx80(LIT64(0xBFF6687E314987D8)); // A3
        fp2 = floatx80_add(fp2, fp1); // A3+V
        fp2 = floatx80_mul(fp2, fp1); // V*(A3+V)
        fp1 = floatx80_mul(fp1, fp0); // U*V
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x4002AC6934A26DB3))); // A2+V*(A3+V)
        fp1 = floatx80_mul(fp1, float64_to_floatx80(LIT64(0xBFC2476F4E1DA28E))); // A1+U*V
        fp1 = floatx80_mul(fp1, fp2); // A1*U*V*(A2+V*(A3+V))
        fp0 = floatx80_add(fp0, fp1); // ATAN(U)
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_add(fp0, fp3); // ATAN(X)
        
        float_raise(float_flag_inexact);
        
        return a;
    }
}

/*----------------------------------------------------------------------------
 | Hyperbolic arc tangent
 *----------------------------------------------------------------------------*/

floatx80 floatx80_atanh(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact;
    floatx80 fp0, fp1, fp2, one;

    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF && (bits64) (aSig<<1)) {
        return propagateFloatx80NaNOneArg(a);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact >= 0x3FFF8000) { // |X| >= 1
        if (aExp == one_exp && aSig == one_sig) { // |X| == 1
            float_raise(float_flag_divbyzero);
            return packFloatx80(aSign, 0x7FFF, floatx80_default_infinity_low);
        } else { // |X| > 1
            float_raise(float_flag_invalid);
            a.low = floatx80_default_nan_low;
            a.high = floatx80_default_nan_high;
            return a;
        }
    } // |X| < 1
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    one = packFloatx80(0, one_exp, one_sig);
    fp2 = packFloatx80(aSign, 0x3FFE, one_sig); // SIGN(X) * (1/2)
    fp0 = packFloatx80(0, aExp, aSig); // Y = |X|
    fp1 = packFloatx80(1, aExp, aSig); // -Y
    fp0 = floatx80_add(fp0, fp0); // 2Y
    fp1 = floatx80_add(fp1, one); // 1-Y
    fp0 = floatx80_div(fp0, fp1); // Z = 2Y/(1-Y)
    fp0 = floatx80_lognp1(fp0); // LOG1P(Z)
    
    float_rounding_mode = user_rnd_mode;
    floatx80_rounding_precision = user_rnd_prec;
    
    a = floatx80_mul(fp0, fp2); // ATANH(X) = SIGN(X) * (1/2) * LOG1P(Z)
    
    float_raise(float_flag_inexact);
    
    return a;
}

/*----------------------------------------------------------------------------
 | Cosine
 *----------------------------------------------------------------------------*/

floatx80 floatx80_cos(floatx80 a)
{
    flag aSign, xSign;
    int32 aExp, xExp;
    bits64 aSig, xSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, l, n, j;
    floatx80 fp0, fp1, fp2, fp3, fp4, fp5, x, invtwopi, twopi1, twopi2;
    float32 posneg1, twoto63;
    flag adjn, endflag;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        float_raise(float_flag_invalid);
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(0, one_exp, one_sig);
    }
    
    adjn = 1;
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    fp0 = a;
    
    if (compact < 0x3FD78000 || compact > 0x4004BC7E) { // 2^(-40) > |X| > 15 PI
        if (compact > 0x3FFF8000) { // |X| >= 15 PI
            // REDUCEX
            fp1 = packFloatx80(0, 0, 0);
            if (compact == 0x7FFEFFFF) {
                twopi1 = packFloatx80(aSign ^ 1, 0x7FFE, LIT64(0xC90FDAA200000000));
                twopi2 = packFloatx80(aSign ^ 1, 0x7FDC, LIT64(0x85A308D300000000));
                fp0 = floatx80_add(fp0, twopi1);
                fp1 = fp0;
                fp0 = floatx80_add(fp0, twopi2);
                fp1 = floatx80_sub(fp1, fp0);
                fp1 = floatx80_add(fp1, twopi2);
            }
        loop:
            xSign = extractFloatx80Sign(fp0);
            xExp = extractFloatx80Exp(fp0);
            xExp -= 0x3FFF;
            if (xExp <= 28) {
                l = 0;
                endflag = 1;
            } else {
                l = xExp - 27;
                endflag = 0;
            }
            invtwopi = packFloatx80(0, 0x3FFE - l, LIT64(0xA2F9836E4E44152A)); // INVTWOPI
            twopi1 = packFloatx80(0, 0x3FFF + l, LIT64(0xC90FDAA200000000));
            twopi2 = packFloatx80(0, 0x3FDD + l, LIT64(0x85A308D300000000));
            
            twoto63 = 0x5F000000;
            twoto63 |= xSign ? 0x80000000 : 0x00000000; // SIGN(INARG)*2^63 IN SGL
            
            fp2 = floatx80_mul(fp0, invtwopi);
            fp2 = floatx80_add(fp2, float32_to_floatx80(twoto63)); // THE FRACTIONAL PART OF FP2 IS ROUNDED
            fp2 = floatx80_sub(fp2, float32_to_floatx80(twoto63)); // FP2 is N
            fp4 = floatx80_mul(twopi1, fp2); // W = N*P1
            fp5 = floatx80_mul(twopi2, fp2); // w = N*P2
            fp3 = floatx80_add(fp4, fp5); // FP3 is P
            fp4 = floatx80_sub(fp4, fp3); // W-P
            fp0 = floatx80_sub(fp0, fp3); // FP0 is A := R - P
            fp4 = floatx80_add(fp4, fp5); // FP4 is p = (W-P)+w
            fp3 = fp0; // FP3 is A
            fp1 = floatx80_sub(fp1, fp4); // FP1 is a := r - p
            fp0 = floatx80_add(fp0, fp1); // FP0 is R := A+a
            
            if (endflag > 0) {
                n = floatx80_to_int32(fp2);
                goto sincont;
            }
            fp3 = floatx80_sub(fp3, fp0); // A-R
            fp1 = floatx80_add(fp1, fp3); // FP1 is r := (A-R)+a
            goto loop;
        } else {
            // SINSM
            fp0 = float32_to_floatx80(0x3F800000); // 1
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            if (adjn) {
                // COSTINY
                a = floatx80_sub(fp0, float32_to_floatx80(0x00800000));
            } else {
                // SINTINY
                a = floatx80_move(a);
            }
            float_raise(float_flag_inexact);
            
            return a;
        }
    } else {
        fp1 = floatx80_mul(fp0, float64_to_floatx80(LIT64(0x3FE45F306DC9C883))); // X*2/PI
        
        n = floatx80_to_int32(fp1);
        j = 32 + n;
        
        fp0 = floatx80_sub(fp0, pi_tbl[j]); // X-Y1
        fp0 = floatx80_sub(fp0, float32_to_floatx80(pi_tbl2[j])); // FP0 IS R = (X-Y1)-Y2
        
    sincont:
        if ((n + adjn) & 1) {
            // COSPOLY
            fp0 = floatx80_mul(fp0, fp0); // FP0 IS S
            fp1 = floatx80_mul(fp0, fp0); // FP1 IS T
            fp2 = float64_to_floatx80(LIT64(0x3D2AC4D0D6011EE3)); // B8
            fp3 = float64_to_floatx80(LIT64(0xBDA9396F9F45AC19)); // B7
            
            xSign = extractFloatx80Sign(fp0); // X IS S
            xExp = extractFloatx80Exp(fp0);
            xSig = extractFloatx80Frac(fp0);
            
            if (((n + adjn) >> 1) & 1) {
                xSign ^= 1;
                posneg1 = 0xBF800000; // -1
            } else {
                xSign ^= 0;
                posneg1 = 0x3F800000; // 1
            } // X IS NOW R'= SGN*R
            
            fp2 = floatx80_mul(fp2, fp1); // TB8
            fp3 = floatx80_mul(fp3, fp1); // TB7
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3E21EED90612C972))); // B6+TB8
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBE927E4FB79D9FCF))); // B5+TB7
            fp2 = floatx80_mul(fp2, fp1); // T(B6+TB8)
            fp3 = floatx80_mul(fp3, fp1); // T(B5+TB7)
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3EFA01A01A01D423))); // B4+T(B6+TB8)
            fp4 = packFloatx80(1, 0x3FF5, LIT64(0xB60B60B60B61D438));
            fp3 = floatx80_add(fp3, fp4); // B3+T(B5+TB7)
            fp2 = floatx80_mul(fp2, fp1); // T(B4+T(B6+TB8))
            fp1 = floatx80_mul(fp1, fp3); // T(B3+T(B5+TB7))
            fp4 = packFloatx80(0, 0x3FFA, LIT64(0xAAAAAAAAAAAAAB5E));
            fp2 = floatx80_add(fp2, fp4); // B2+T(B4+T(B6+TB8))
            fp1 = floatx80_add(fp1, float32_to_floatx80(0xBF000000)); // B1+T(B3+T(B5+TB7))
            fp0 = floatx80_mul(fp0, fp2); // S(B2+T(B4+T(B6+TB8)))
            fp0 = floatx80_add(fp0, fp1); // [B1+T(B3+T(B5+TB7))]+[S(B2+T(B4+T(B6+TB8)))]
            
            x = packFloatx80(xSign, xExp, xSig);
            fp0 = floatx80_mul(fp0, x);
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_add(fp0, float32_to_floatx80(posneg1));
            
            float_raise(float_flag_inexact);
            
            return a;
        } else {
            // SINPOLY
            xSign = extractFloatx80Sign(fp0); // X IS R
            xExp = extractFloatx80Exp(fp0);
            xSig = extractFloatx80Frac(fp0);
            
            xSign ^= ((n + adjn) >> 1) & 1; // X IS NOW R'= SGN*R
            
            fp0 = floatx80_mul(fp0, fp0); // FP0 IS S
            fp1 = floatx80_mul(fp0, fp0); // FP1 IS T
            fp3 = float64_to_floatx80(LIT64(0xBD6AAA77CCC994F5)); // A7
            fp2 = float64_to_floatx80(LIT64(0x3DE612097AAE8DA1)); // A6
            fp3 = floatx80_mul(fp3, fp1); // T*A7
            fp2 = floatx80_mul(fp2, fp1); // T*A6
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBE5AE6452A118AE4))); // A5+T*A7
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3EC71DE3A5341531))); // A4+T*A6
            fp3 = floatx80_mul(fp3, fp1); // T(A5+TA7)
            fp2 = floatx80_mul(fp2, fp1); // T(A4+TA6)
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBF2A01A01A018B59))); // A3+T(A5+TA7)
            fp4 = packFloatx80(0, 0x3FF8, LIT64(0x88888888888859AF));
            fp2 = floatx80_add(fp2, fp4); // A2+T(A4+TA6)
            fp1 = floatx80_mul(fp1, fp3); // T(A3+T(A5+TA7))
            fp2 = floatx80_mul(fp2, fp0); // S(A2+T(A4+TA6))
            fp4 = packFloatx80(1, 0x3FFC, LIT64(0xAAAAAAAAAAAAAA99));
            fp1 = floatx80_add(fp1, fp4); // A1+T(A3+T(A5+TA7))
            fp1 = floatx80_add(fp1, fp2); // [A1+T(A3+T(A5+TA7))]+[S(A2+T(A4+TA6))]
            
            x = packFloatx80(xSign, xExp, xSig);
            fp0 = floatx80_mul(fp0, x); // R'*S
            fp0 = floatx80_mul(fp0, fp1); // SIN(R')-R'
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_add(fp0, x);
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    }
}

/*----------------------------------------------------------------------------
 | Hyperbolic cosine
 *----------------------------------------------------------------------------*/

floatx80 floatx80_cosh(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact;
    floatx80 fp0, fp1;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(0, one_exp, one_sig);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact > 0x400CB167) {
        if (compact > 0x400CB2B3) {
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            return roundAndPackFloatx80(floatx80_rounding_precision, 0, 0x8000, one_sig, 0);
        } else {
            fp0 = packFloatx80(0, aExp, aSig);
            fp0 = floatx80_sub(fp0, float64_to_floatx80(LIT64(0x40C62D38D3D64634)));
            fp0 = floatx80_sub(fp0, float64_to_floatx80(LIT64(0x3D6F90AEB1E75CC7)));
            fp0 = floatx80_etox(fp0);
            fp1 = packFloatx80(0, 0x7FFB, one_sig);
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;

            a = floatx80_mul(fp0, fp1);
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    }
    
    fp0 = packFloatx80(0, aExp, aSig); // |X|
    fp0 = floatx80_etox(fp0); // EXP(|X|)
    fp0 = floatx80_mul(fp0, float32_to_floatx80(0x3F000000)); // (1/2)*EXP(|X|)
    fp1 = float32_to_floatx80(0x3E800000); // 1/4
    fp1 = floatx80_div(fp1, fp0); // 1/(2*EXP(|X|))
    
    float_rounding_mode = user_rnd_mode;
    floatx80_rounding_precision = user_rnd_prec;
    
    a = floatx80_add(fp0, fp1);
    
    float_raise(float_flag_inexact);
    
    return a;
}

/*----------------------------------------------------------------------------
 | e to x
 *----------------------------------------------------------------------------*/

floatx80 floatx80_etox(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, n, j, k, m, m1;
    floatx80 fp0, fp1, fp2, fp3, l2, scale, adjscale;
    flag adjflag;

    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        if (aSign) return packFloatx80(0, 0, 0);
        return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(0, one_exp, one_sig);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    adjflag = 0;
    
    if (aExp >= 0x3FBE) { // |X| >= 2^(-65)
        compact = floatx80_make_compact(aExp, aSig);
        
        if (compact < 0x400CB167) { // |X| < 16380 log2
            fp0 = a;
            fp1 = a;
            fp0 = floatx80_mul(fp0, float32_to_floatx80(0x42B8AA3B)); // 64/log2 * X
            adjflag = 0;
            n = floatx80_to_int32(fp0); // int(64/log2*X)
            fp0 = int32_to_floatx80(n);
            
            j = n & 0x3F; // J = N mod 64
            m = n / 64; // NOTE: this is really arithmetic right shift by 6
            if (n < 0 && j) { // arithmetic right shift is division and round towards minus infinity
                m--;
            }
            m += 0x3FFF; // biased exponent of 2^(M)
            
        expcont1:
            fp2 = fp0; // N
            fp0 = floatx80_mul(fp0, float32_to_floatx80(0xBC317218)); // N * L1, L1 = lead(-log2/64)
            l2 = packFloatx80(0, 0x3FDC, LIT64(0x82E308654361C4C6));
            fp2 = floatx80_mul(fp2, l2); // N * L2, L1+L2 = -log2/64
            fp0 = floatx80_add(fp0, fp1); // X + N*L1
            fp0 = floatx80_add(fp0, fp2); // R
            
            fp1 = floatx80_mul(fp0, fp0); // S = R*R
            fp2 = float32_to_floatx80(0x3AB60B70); // A5
            fp2 = floatx80_mul(fp2, fp1); // fp2 is S*A5
            fp3 = floatx80_mul(float32_to_floatx80(0x3C088895), fp1); // fp3 is S*A4
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FA5555555554431))); // fp2 is A3+S*A5
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0x3FC5555555554018))); // fp3 is A2+S*A4
            fp2 = floatx80_mul(fp2, fp1); // fp2 is S*(A3+S*A5)
            fp3 = floatx80_mul(fp3, fp1); // fp3 is S*(A2+S*A4)
            fp2 = floatx80_add(fp2, float32_to_floatx80(0x3F000000)); // fp2 is A1+S*(A3+S*A5)
            fp3 = floatx80_mul(fp3, fp0); // fp3 IS R*S*(A2+S*A4)
            fp2 = floatx80_mul(fp2, fp1); // fp2 IS S*(A1+S*(A3+S*A5))
            fp0 = floatx80_add(fp0, fp3); // fp0 IS R+R*S*(A2+S*A4)
            fp0 = floatx80_add(fp0, fp2); // fp0 IS EXP(R) - 1
            
            fp1 = exp_tbl[j];
            fp0 = floatx80_mul(fp0, fp1); // 2^(J/64)*(Exp(R)-1)
            fp0 = floatx80_add(fp0, float32_to_floatx80(exp_tbl2[j])); // accurate 2^(J/64)
            fp0 = floatx80_add(fp0, fp1); // 2^(J/64) + 2^(J/64)*(Exp(R)-1)
            
            scale = packFloatx80(0, m, one_sig);
            if (adjflag) {
                adjscale = packFloatx80(0, m1, one_sig);
                fp0 = floatx80_mul(fp0, adjscale);
            }
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_mul(fp0, scale);
            
            float_raise(float_flag_inexact);
            
            return a;
        } else { // |X| >= 16380 log2
            if (compact > 0x400CB27C) { // |X| >= 16480 log2
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                if (aSign) {
                    a = roundAndPackFloatx80(floatx80_rounding_precision, 0, -0x1000, aSig, 0);
                } else {
                    a = roundAndPackFloatx80(floatx80_rounding_precision, 0, 0x8000, aSig, 0);
                }
                float_raise(float_flag_inexact);
                
                return a;
            } else {
                fp0 = a;
                fp1 = a;
                fp0 = floatx80_mul(fp0, float32_to_floatx80(0x42B8AA3B)); // 64/log2 * X
                adjflag = 1;
                n = floatx80_to_int32(fp0); // int(64/log2*X)
                fp0 = int32_to_floatx80(n);
                
                j = n & 0x3F; // J = N mod 64
                k = n / 64; // NOTE: this is really arithmetic right shift by 6
                if (n < 0 && j) { // arithmetic right shift is division and round towards minus infinity
                    k--;
                }
                m1 = k / 2; // NOTE: this is really arithmetic right shift by 1
                if (k < 0 && (k & 1)) { // arithmetic right shift is division and round towards minus infinity
                    m1--;
                }
                m = k - m1;
                m1 += 0x3FFF; // biased exponent of 2^(M1)
                m += 0x3FFF; // biased exponent of 2^(M)
                
                goto expcont1;
            }
        }
    } else { // |X| < 2^(-65)
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_add(a, float32_to_floatx80(0x3F800000)); // 1 + X
        
        float_raise(float_flag_inexact);
        
        return a;
    }
}

/*----------------------------------------------------------------------------
 | e to x minus 1
 *----------------------------------------------------------------------------*/

floatx80 floatx80_etoxm1(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, n, j, m, m1;
    floatx80 fp0, fp1, fp2, fp3, l2, sc, onebysc;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        if (aSign) return packFloatx80(aSign, one_exp, one_sig);
        return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    if (aExp >= 0x3FFD) { // |X| >= 1/4
        compact = floatx80_make_compact(aExp, aSig);
        
        if (compact <= 0x4004C215) { // |X| <= 70 log2
            fp0 = a;
            fp1 = a;
            fp0 = floatx80_mul(fp0, float32_to_floatx80(0x42B8AA3B)); // 64/log2 * X
            n = floatx80_to_int32(fp0); // int(64/log2*X)
            fp0 = int32_to_floatx80(n);
            
            j = n & 0x3F; // J = N mod 64
            m = n / 64; // NOTE: this is really arithmetic right shift by 6
            if (n < 0 && j) { // arithmetic right shift is division and round towards minus infinity
                m--;
            }
            m1 = -m;
            //m += 0x3FFF; // biased exponent of 2^(M)
            //m1 += 0x3FFF; // biased exponent of -2^(-M)
            
            fp2 = fp0; // N
            fp0 = floatx80_mul(fp0, float32_to_floatx80(0xBC317218)); // N * L1, L1 = lead(-log2/64)
            l2 = packFloatx80(0, 0x3FDC, LIT64(0x82E308654361C4C6));
            fp2 = floatx80_mul(fp2, l2); // N * L2, L1+L2 = -log2/64
            fp0 = floatx80_add(fp0, fp1); // X + N*L1
            fp0 = floatx80_add(fp0, fp2); // R
            
            fp1 = floatx80_mul(fp0, fp0); // S = R*R
            fp2 = float32_to_floatx80(0x3950097B); // A6
            fp2 = floatx80_mul(fp2, fp1); // fp2 is S*A6
            fp3 = floatx80_mul(float32_to_floatx80(0x3AB60B6A), fp1); // fp3 is S*A5
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3F81111111174385))); // fp2 IS A4+S*A6
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0x3FA5555555554F5A))); // fp3 is A3+S*A5
            fp2 = floatx80_mul(fp2, fp1); // fp2 IS S*(A4+S*A6)
            fp3 = floatx80_mul(fp3, fp1); // fp3 IS S*(A3+S*A5)
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FC5555555555555))); // fp2 IS A2+S*(A4+S*A6)
            fp3 = floatx80_add(fp3, float32_to_floatx80(0x3F000000)); // fp3 IS A1+S*(A3+S*A5)
            fp2 = floatx80_mul(fp2, fp1); // fp2 IS S*(A2+S*(A4+S*A6))
            fp1 = floatx80_mul(fp1, fp3); // fp1 IS S*(A1+S*(A3+S*A5))
            fp2 = floatx80_mul(fp2, fp0); // fp2 IS R*S*(A2+S*(A4+S*A6))
            fp0 = floatx80_add(fp0, fp1); // fp0 IS R+S*(A1+S*(A3+S*A5))
            fp0 = floatx80_add(fp0, fp2); // fp0 IS EXP(R) - 1
            
            fp0 = floatx80_mul(fp0, exp_tbl[j]); // 2^(J/64)*(Exp(R)-1)
            
            if (m >= 64) {
                fp1 = float32_to_floatx80(exp_tbl2[j]);
                onebysc = packFloatx80(1, m1 + 0x3FFF, one_sig); // -2^(-M)
                fp1 = floatx80_add(fp1, onebysc);
                fp0 = floatx80_add(fp0, fp1);
                fp0 = floatx80_add(fp0, exp_tbl[j]);
            } else if (m < -3) {
                fp0 = floatx80_add(fp0, float32_to_floatx80(exp_tbl2[j]));
                fp0 = floatx80_add(fp0, exp_tbl[j]);
                onebysc = packFloatx80(1, m1 + 0x3FFF, one_sig); // -2^(-M)
                fp0 = floatx80_add(fp0, onebysc);
            } else { // -3 <= m <= 63
                fp1 = exp_tbl[j];
                fp0 = floatx80_add(fp0, float32_to_floatx80(exp_tbl2[j]));
                onebysc = packFloatx80(1, m1 + 0x3FFF, one_sig); // -2^(-M)
                fp1 = floatx80_add(fp1, onebysc);
                fp0 = floatx80_add(fp0, fp1);
            }
            
            sc = packFloatx80(0, m + 0x3FFF, one_sig);
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_mul(fp0, sc);
            
            float_raise(float_flag_inexact);
            
            return a;
        } else { // |X| > 70 log2
            if (aSign) {
                fp0 = float32_to_floatx80(0xBF800000); // -1
                
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                a = floatx80_add(fp0, float32_to_floatx80(0x00800000)); // -1 + 2^(-126)
                
                float_raise(float_flag_inexact);
                
                return a;
            } else {
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                return floatx80_etox(a);
            }
        }
    } else { // |X| < 1/4
        if (aExp >= 0x3FBE) {
            fp0 = a;
            fp0 = floatx80_mul(fp0, fp0); // S = X*X
            fp1 = float32_to_floatx80(0x2F30CAA8); // B12
            fp1 = floatx80_mul(fp1, fp0); // S * B12
            fp2 = float32_to_floatx80(0x310F8290); // B11
            fp1 = floatx80_add(fp1, float32_to_floatx80(0x32D73220)); // B10
            fp2 = floatx80_mul(fp2, fp0);
            fp1 = floatx80_mul(fp1, fp0);
            fp2 = floatx80_add(fp2, float32_to_floatx80(0x3493F281)); // B9
            fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3EC71DE3A5774682))); // B8
            fp2 = floatx80_mul(fp2, fp0);
            fp1 = floatx80_mul(fp1, fp0);
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3EFA01A019D7CB68))); // B7
            fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3F2A01A01A019DF3))); // B6
            fp2 = floatx80_mul(fp2, fp0);
            fp1 = floatx80_mul(fp1, fp0);
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3F56C16C16C170E2))); // B5
            fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3F81111111111111))); // B4
            fp2 = floatx80_mul(fp2, fp0);
            fp1 = floatx80_mul(fp1, fp0);
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FA5555555555555))); // B3
            fp3 = packFloatx80(0, 0x3FFC, LIT64(0xAAAAAAAAAAAAAAAB));
            fp1 = floatx80_add(fp1, fp3); // B2
            fp2 = floatx80_mul(fp2, fp0);
            fp1 = floatx80_mul(fp1, fp0);
            
            fp2 = floatx80_mul(fp2, fp0);
            fp1 = floatx80_mul(fp1, a);
            
            fp0 = floatx80_mul(fp0, float32_to_floatx80(0x3F000000)); // S*B1
            fp1 = floatx80_add(fp1, fp2); // Q
            fp0 = floatx80_add(fp0, fp1); // S*B1+Q
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_add(fp0, a);
            
            float_raise(float_flag_inexact);
            
            return a;
        } else { // |X| < 2^(-65)
            sc = packFloatx80(1, 1, one_sig);
            fp0 = a;

            if (aExp < 0x0033) { // |X| < 2^(-16382)
                fp0 = floatx80_mul(fp0, float64_to_floatx80(LIT64(0x48B0000000000000)));
                fp0 = floatx80_add(fp0, sc);
                
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;

                a = floatx80_mul(fp0, float64_to_floatx80(LIT64(0x3730000000000000)));
            } else {
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                a = floatx80_add(fp0, sc);
            }
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    }
}

/*----------------------------------------------------------------------------
 | Log base 10
 *----------------------------------------------------------------------------*/

floatx80 floatx80_log10(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    floatx80 fp0, fp1;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) propagateFloatx80NaNOneArg(a);
        if (aSign == 0)
            return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        float_raise(float_flag_divbyzero);
        return packFloatx80(1, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aSign) {
        float_raise(float_flag_invalid);
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    fp0 = floatx80_logn(a);
    fp1 = packFloatx80(0, 0x3FFD, LIT64(0xDE5BD8A937287195)); // INV_L10
    
    float_rounding_mode = user_rnd_mode;
    floatx80_rounding_precision = user_rnd_prec;
    
    a = floatx80_mul(fp0, fp1); // LOGN(X)*INV_L10
    
    float_raise(float_flag_inexact);
    
    return a;
}

/*----------------------------------------------------------------------------
 | Log base 2
 *----------------------------------------------------------------------------*/

floatx80 floatx80_log2(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    floatx80 fp0, fp1;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) propagateFloatx80NaNOneArg(a);
        if (aSign == 0)
            return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0) {
        if (aSig == 0) {
            float_raise(float_flag_divbyzero);
            return packFloatx80(1, 0x7FFF, floatx80_default_infinity_low);
        }
        normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
    }
    
    if (aSign) {
        float_raise(float_flag_invalid);
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    if (aSig == one_sig) { // X is 2^k
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = int32_to_floatx80(aExp-0x3FFF);
    } else {
        fp0 = floatx80_logn(a);
        fp1 = packFloatx80(0, 0x3FFF, LIT64(0xB8AA3B295C17F0BC)); // INV_L2
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_mul(fp0, fp1); // LOGN(X)*INV_L2
    }
    
    float_raise(float_flag_inexact);
    
    return a;
}

/*----------------------------------------------------------------------------
 | Log base e
 *----------------------------------------------------------------------------*/

floatx80 floatx80_logn(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig, fSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, j, k, adjk;
    floatx80 fp0, fp1, fp2, fp3, f, logof2, klog2, saveu;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) propagateFloatx80NaNOneArg(a);
        if (aSign == 0)
            return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    adjk = 0;
    
    if (aExp == 0) {
        if (aSig == 0) { // zero
            float_raise(float_flag_divbyzero);
            return packFloatx80(1, 0x7FFF, floatx80_default_infinity_low);
        }
#if 1
        if ((aSig & one_sig) == 0) { // denormal
            normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
            adjk = -100;
            aExp += 100;
            a = packFloatx80(aSign, aExp, aSig);
        }
#else
        normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
#endif
    }
    
    if (aSign) {
        float_raise(float_flag_invalid);
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact < 0x3FFEF07D || compact > 0x3FFF8841) { // |X| < 15/16 or |X| > 17/16
        k = aExp - 0x3FFF;
        k += adjk;
        fp1 = int32_to_floatx80(k);
        
        fSig = (aSig & LIT64(0xFE00000000000000)) | LIT64(0x0100000000000000);
        j = (fSig >> 56) & 0x7E; // DISPLACEMENT FOR 1/F
        
        f = packFloatx80(0, 0x3FFF, fSig); // F
        fp0 = packFloatx80(0, 0x3FFF, aSig); // Y
        
        fp0 = floatx80_sub(fp0, f); // Y-F
        
        // LP1CONT1
        fp0 = floatx80_mul(fp0, log_tbl[j]); // FP0 IS U = (Y-F)/F
        logof2 = packFloatx80(0, 0x3FFE, LIT64(0xB17217F7D1CF79AC));
        klog2 = floatx80_mul(fp1, logof2); // FP1 IS K*LOG2
        fp2 = floatx80_mul(fp0, fp0); // FP2 IS V=U*U
        
        fp3 = fp2;
        fp1 = fp2;
        
        fp1 = floatx80_mul(fp1, float64_to_floatx80(LIT64(0x3FC2499AB5E4040B))); // V*A6
        fp2 = floatx80_mul(fp2, float64_to_floatx80(LIT64(0xBFC555B5848CB7DB))); // V*A5
        fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3FC99999987D8730))); // A4+V*A6
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0xBFCFFFFFFF6F7E97))); // A3+V*A5
        fp1 = floatx80_mul(fp1, fp3); // V*(A4+V*A6)
        fp2 = floatx80_mul(fp2, fp3); // V*(A3+V*A5)
        fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3FD55555555555A4))); // A2+V*(A4+V*A6)
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0xBFE0000000000008))); // A1+V*(A3+V*A5)
        fp1 = floatx80_mul(fp1, fp3); // V*(A2+V*(A4+V*A6))
        fp2 = floatx80_mul(fp2, fp3); // V*(A1+V*(A3+V*A5))
        fp1 = floatx80_mul(fp1, fp0); // U*V*(A2+V*(A4+V*A6))
        fp0 = floatx80_add(fp0, fp2); // U+V*(A1+V*(A3+V*A5))
        
        fp1 = floatx80_add(fp1, log_tbl[j+1]); // LOG(F)+U*V*(A2+V*(A4+V*A6))
        fp0 = floatx80_add(fp0, fp1); // FP0 IS LOG(F) + LOG(1+U)
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_add(fp0, klog2);
        
        float_raise(float_flag_inexact);
        
        return a;
    } else { // |X-1| >= 1/16
        fp0 = a;
        fp1 = a;
        fp1 = floatx80_sub(fp1, float32_to_floatx80(0x3F800000)); // FP1 IS X-1
        fp0 = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // FP0 IS X+1
        fp1 = floatx80_add(fp1, fp1); // FP1 IS 2(X-1)
        
        // LP1CONT2
        fp1 = floatx80_div(fp1, fp0); // U
        saveu = fp1;
        fp0 = floatx80_mul(fp1, fp1); // FP0 IS V = U*U
        fp1 = floatx80_mul(fp0, fp0); // FP1 IS W = V*V
        
        fp3 = float64_to_floatx80(LIT64(0x3F175496ADD7DAD6)); // B5
        fp2 = float64_to_floatx80(LIT64(0x3F3C71C2FE80C7E0)); // B4
        fp3 = floatx80_mul(fp3, fp1); // W*B5
        fp2 = floatx80_mul(fp2, fp1); // W*B4
        fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0x3F624924928BCCFF))); // B3+W*B5
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3F899999999995EC))); // B2+W*B4
        fp1 = floatx80_mul(fp1, fp3); // W*(B3+W*B5)
        fp2 = floatx80_mul(fp2, fp0); // V*(B2+W*B4)
        fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3FB5555555555555))); // B1+W*(B3+W*B5)
        
        fp0 = floatx80_mul(fp0, saveu); // FP0 IS U*V
        fp1 = floatx80_add(fp1, fp2); // B1+W*(B3+W*B5) + V*(B2+W*B4)
        fp0 = floatx80_mul(fp0, fp1); // U*V*( [B1+W*(B3+W*B5)] + [V*(B2+W*B4)] )
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_add(fp0, saveu);
        
        //if (!floatx80_is_zero(a)) {
            float_raise(float_flag_inexact);
        //}
        
        return a;
    }
}

/*----------------------------------------------------------------------------
 | Log base e of x plus 1
 *----------------------------------------------------------------------------*/

floatx80 floatx80_lognp1(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig, fSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, j, k;
    floatx80 fp0, fp1, fp2, fp3, f, logof2, klog2, saveu;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) propagateFloatx80NaNOneArg(a);
        if (aSign) {
            float_raise(float_flag_invalid);
            a.low = floatx80_default_nan_low;
            a.high = floatx80_default_nan_high;
            return a;
        }
        return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    if (aSign && aExp >= one_exp) {
        if (aExp == one_exp && aSig == one_sig) {
            float_raise(float_flag_divbyzero);
            packFloatx80(aSign, 0x7FFF, floatx80_default_infinity_low);
        }
        float_raise(float_flag_invalid);
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    if (aExp < 0x3f99 || (aExp == 0x3f99 && aSig == one_sig)) { // <= min threshold
        float_raise(float_flag_inexact);
        return floatx80_move(a);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    fp0 = a; // Z
    fp1 = a;
    
    fp0 = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // X = (1+Z)
    
    aExp = extractFloatx80Exp(fp0);
    aSig = extractFloatx80Frac(fp0);
    
    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact < 0x3FFE8000 || compact > 0x3FFFC000) { // |X| < 1/2 or |X| > 3/2
        k = aExp - 0x3FFF;
        fp1 = int32_to_floatx80(k);
        
        fSig = (aSig & LIT64(0xFE00000000000000)) | LIT64(0x0100000000000000);
        j = (fSig >> 56) & 0x7E; // DISPLACEMENT FOR 1/F
        
        f = packFloatx80(0, 0x3FFF, fSig); // F
        fp0 = packFloatx80(0, 0x3FFF, aSig); // Y
        
        fp0 = floatx80_sub(fp0, f); // Y-F
        
    lp1cont1:
        // LP1CONT1
        fp0 = floatx80_mul(fp0, log_tbl[j]); // FP0 IS U = (Y-F)/F
        logof2 = packFloatx80(0, 0x3FFE, LIT64(0xB17217F7D1CF79AC));
        klog2 = floatx80_mul(fp1, logof2); // FP1 IS K*LOG2
        fp2 = floatx80_mul(fp0, fp0); // FP2 IS V=U*U
        
        fp3 = fp2;
        fp1 = fp2;
        
        fp1 = floatx80_mul(fp1, float64_to_floatx80(LIT64(0x3FC2499AB5E4040B))); // V*A6
        fp2 = floatx80_mul(fp2, float64_to_floatx80(LIT64(0xBFC555B5848CB7DB))); // V*A5
        fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3FC99999987D8730))); // A4+V*A6
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0xBFCFFFFFFF6F7E97))); // A3+V*A5
        fp1 = floatx80_mul(fp1, fp3); // V*(A4+V*A6)
        fp2 = floatx80_mul(fp2, fp3); // V*(A3+V*A5)
        fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3FD55555555555A4))); // A2+V*(A4+V*A6)
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0xBFE0000000000008))); // A1+V*(A3+V*A5)
        fp1 = floatx80_mul(fp1, fp3); // V*(A2+V*(A4+V*A6))
        fp2 = floatx80_mul(fp2, fp3); // V*(A1+V*(A3+V*A5))
        fp1 = floatx80_mul(fp1, fp0); // U*V*(A2+V*(A4+V*A6))
        fp0 = floatx80_add(fp0, fp2); // U+V*(A1+V*(A3+V*A5))
        
        fp1 = floatx80_add(fp1, log_tbl[j+1]); // LOG(F)+U*V*(A2+V*(A4+V*A6))
        fp0 = floatx80_add(fp0, fp1); // FP0 IS LOG(F) + LOG(1+U)
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_add(fp0, klog2);
        
        float_raise(float_flag_inexact);
        
        return a;
    } else if (compact < 0x3FFEF07D || compact > 0x3FFF8841) { // |X| < 1/16 or |X| > -1/16
        // LP1CARE
        fSig = (aSig & LIT64(0xFE00000000000000)) | LIT64(0x0100000000000000);
        f = packFloatx80(0, 0x3FFF, fSig); // F
        j = (fSig >> 56) & 0x7E; // DISPLACEMENT FOR 1/F

        if (compact >= 0x3FFF8000) { // 1+Z >= 1
            // KISZERO
            fp0 = floatx80_sub(float32_to_floatx80(0x3F800000), f); // 1-F
            fp0 = floatx80_add(fp0, fp1); // FP0 IS Y-F = (1-F)+Z
            fp1 = packFloatx80(0, 0, 0); // K = 0
        } else {
            // KISNEG
            fp0 = floatx80_sub(float32_to_floatx80(0x40000000), f); // 2-F
            fp1 = floatx80_add(fp1, fp1); // 2Z
            fp0 = floatx80_add(fp0, fp1); // FP0 IS Y-F = (2-F)+2Z
            fp1 = packFloatx80(1, one_exp, one_sig); // K = -1
        }
        goto lp1cont1;
    } else {
        // LP1ONE16
        fp1 = floatx80_add(fp1, fp1); // FP1 IS 2Z
        fp0 = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // FP0 IS 1+X
        
        // LP1CONT2
        fp1 = floatx80_div(fp1, fp0); // U
        saveu = fp1;
        fp0 = floatx80_mul(fp1, fp1); // FP0 IS V = U*U
        fp1 = floatx80_mul(fp0, fp0); // FP1 IS W = V*V
        
        fp3 = float64_to_floatx80(LIT64(0x3F175496ADD7DAD6)); // B5
        fp2 = float64_to_floatx80(LIT64(0x3F3C71C2FE80C7E0)); // B4
        fp3 = floatx80_mul(fp3, fp1); // W*B5
        fp2 = floatx80_mul(fp2, fp1); // W*B4
        fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0x3F624924928BCCFF))); // B3+W*B5
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3F899999999995EC))); // B2+W*B4
        fp1 = floatx80_mul(fp1, fp3); // W*(B3+W*B5)
        fp2 = floatx80_mul(fp2, fp0); // V*(B2+W*B4)
        fp1 = floatx80_add(fp1, float64_to_floatx80(LIT64(0x3FB5555555555555))); // B1+W*(B3+W*B5)
        
        fp0 = floatx80_mul(fp0, saveu); // FP0 IS U*V
        fp1 = floatx80_add(fp1, fp2); // B1+W*(B3+W*B5) + V*(B2+W*B4)
        fp0 = floatx80_mul(fp0, fp1); // U*V*( [B1+W*(B3+W*B5)] + [V*(B2+W*B4)] )
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_add(fp0, saveu);
        
        //if (!floatx80_is_zero(a)) {
            float_raise(float_flag_inexact);
        //}
        
        return a;
    }
}

/*----------------------------------------------------------------------------
 | Sine
 *----------------------------------------------------------------------------*/

floatx80 floatx80_sin(floatx80 a)
{
    flag aSign, xSign;
    int32 aExp, xExp;
    bits64 aSig, xSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, l, n, j;
    floatx80 fp0, fp1, fp2, fp3, fp4, fp5, x, invtwopi, twopi1, twopi2;
    float32 posneg1, twoto63;
    flag adjn, endflag;

    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        float_raise(float_flag_invalid);
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    adjn = 0;
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    fp0 = a;
    
    if (compact < 0x3FD78000 || compact > 0x4004BC7E) { // 2^(-40) > |X| > 15 PI
        if (compact > 0x3FFF8000) { // |X| >= 15 PI
            // REDUCEX
            fp1 = packFloatx80(0, 0, 0);
            if (compact == 0x7FFEFFFF) {
                twopi1 = packFloatx80(aSign ^ 1, 0x7FFE, LIT64(0xC90FDAA200000000));
                twopi2 = packFloatx80(aSign ^ 1, 0x7FDC, LIT64(0x85A308D300000000));
                fp0 = floatx80_add(fp0, twopi1);
                fp1 = fp0;
                fp0 = floatx80_add(fp0, twopi2);
                fp1 = floatx80_sub(fp1, fp0);
                fp1 = floatx80_add(fp1, twopi2);
            }
        loop:
            xSign = extractFloatx80Sign(fp0);
            xExp = extractFloatx80Exp(fp0);
            xExp -= 0x3FFF;
            if (xExp <= 28) {
                l = 0;
                endflag = 1;
            } else {
                l = xExp - 27;
                endflag = 0;
            }
            invtwopi = packFloatx80(0, 0x3FFE - l, LIT64(0xA2F9836E4E44152A)); // INVTWOPI
            twopi1 = packFloatx80(0, 0x3FFF + l, LIT64(0xC90FDAA200000000));
            twopi2 = packFloatx80(0, 0x3FDD + l, LIT64(0x85A308D300000000));
            
            twoto63 = 0x5F000000;
            twoto63 |= xSign ? 0x80000000 : 0x00000000; // SIGN(INARG)*2^63 IN SGL
            
            fp2 = floatx80_mul(fp0, invtwopi);
            fp2 = floatx80_add(fp2, float32_to_floatx80(twoto63)); // THE FRACTIONAL PART OF FP2 IS ROUNDED
            fp2 = floatx80_sub(fp2, float32_to_floatx80(twoto63)); // FP2 is N
            fp4 = floatx80_mul(twopi1, fp2); // W = N*P1
            fp5 = floatx80_mul(twopi2, fp2); // w = N*P2
            fp3 = floatx80_add(fp4, fp5); // FP3 is P
            fp4 = floatx80_sub(fp4, fp3); // W-P
            fp0 = floatx80_sub(fp0, fp3); // FP0 is A := R - P
            fp4 = floatx80_add(fp4, fp5); // FP4 is p = (W-P)+w
            fp3 = fp0; // FP3 is A
            fp1 = floatx80_sub(fp1, fp4); // FP1 is a := r - p
            fp0 = floatx80_add(fp0, fp1); // FP0 is R := A+a
            
            if (endflag > 0) {
                n = floatx80_to_int32(fp2);
                goto sincont;
            }
            fp3 = floatx80_sub(fp3, fp0); // A-R
            fp1 = floatx80_add(fp1, fp3); // FP1 is r := (A-R)+a
            goto loop;
        } else {
            // SINSM
            fp0 = float32_to_floatx80(0x3F800000); // 1
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            if (adjn) {
                // COSTINY
                a = floatx80_sub(fp0, float32_to_floatx80(0x00800000));
            } else {
                // SINTINY
                a = floatx80_move(a);
            }
            float_raise(float_flag_inexact);
            
            return a;
        }
    } else {
        fp1 = floatx80_mul(fp0, float64_to_floatx80(LIT64(0x3FE45F306DC9C883))); // X*2/PI
        
        n = floatx80_to_int32(fp1);
        j = 32 + n;
        
        fp0 = floatx80_sub(fp0, pi_tbl[j]); // X-Y1
        fp0 = floatx80_sub(fp0, float32_to_floatx80(pi_tbl2[j])); // FP0 IS R = (X-Y1)-Y2
        
    sincont:
        if ((n + adjn) & 1) {
            // COSPOLY
            fp0 = floatx80_mul(fp0, fp0); // FP0 IS S
            fp1 = floatx80_mul(fp0, fp0); // FP1 IS T
            fp2 = float64_to_floatx80(LIT64(0x3D2AC4D0D6011EE3)); // B8
            fp3 = float64_to_floatx80(LIT64(0xBDA9396F9F45AC19)); // B7
            
            xSign = extractFloatx80Sign(fp0); // X IS S
            xExp = extractFloatx80Exp(fp0);
            xSig = extractFloatx80Frac(fp0);
            
            if (((n + adjn) >> 1) & 1) {
                xSign ^= 1;
                posneg1 = 0xBF800000; // -1
            } else {
                xSign ^= 0;
                posneg1 = 0x3F800000; // 1
            } // X IS NOW R'= SGN*R
            
            fp2 = floatx80_mul(fp2, fp1); // TB8
            fp3 = floatx80_mul(fp3, fp1); // TB7
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3E21EED90612C972))); // B6+TB8
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBE927E4FB79D9FCF))); // B5+TB7
            fp2 = floatx80_mul(fp2, fp1); // T(B6+TB8)
            fp3 = floatx80_mul(fp3, fp1); // T(B5+TB7)
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3EFA01A01A01D423))); // B4+T(B6+TB8)
            fp4 = packFloatx80(1, 0x3FF5, LIT64(0xB60B60B60B61D438));
            fp3 = floatx80_add(fp3, fp4); // B3+T(B5+TB7)
            fp2 = floatx80_mul(fp2, fp1); // T(B4+T(B6+TB8))
            fp1 = floatx80_mul(fp1, fp3); // T(B3+T(B5+TB7))
            fp4 = packFloatx80(0, 0x3FFA, LIT64(0xAAAAAAAAAAAAAB5E));
            fp2 = floatx80_add(fp2, fp4); // B2+T(B4+T(B6+TB8))
            fp1 = floatx80_add(fp1, float32_to_floatx80(0xBF000000)); // B1+T(B3+T(B5+TB7))
            fp0 = floatx80_mul(fp0, fp2); // S(B2+T(B4+T(B6+TB8)))
            fp0 = floatx80_add(fp0, fp1); // [B1+T(B3+T(B5+TB7))]+[S(B2+T(B4+T(B6+TB8)))]
            
            x = packFloatx80(xSign, xExp, xSig);
            fp0 = floatx80_mul(fp0, x);
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_add(fp0, float32_to_floatx80(posneg1));
            
            float_raise(float_flag_inexact);
            
            return a;
        } else {
            // SINPOLY
            xSign = extractFloatx80Sign(fp0); // X IS R
            xExp = extractFloatx80Exp(fp0);
            xSig = extractFloatx80Frac(fp0);
            
            xSign ^= ((n + adjn) >> 1) & 1; // X IS NOW R'= SGN*R
            
            fp0 = floatx80_mul(fp0, fp0); // FP0 IS S
            fp1 = floatx80_mul(fp0, fp0); // FP1 IS T
            fp3 = float64_to_floatx80(LIT64(0xBD6AAA77CCC994F5)); // A7
            fp2 = float64_to_floatx80(LIT64(0x3DE612097AAE8DA1)); // A6
            fp3 = floatx80_mul(fp3, fp1); // T*A7
            fp2 = floatx80_mul(fp2, fp1); // T*A6
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBE5AE6452A118AE4))); // A5+T*A7
            fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3EC71DE3A5341531))); // A4+T*A6
            fp3 = floatx80_mul(fp3, fp1); // T(A5+TA7)
            fp2 = floatx80_mul(fp2, fp1); // T(A4+TA6)
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBF2A01A01A018B59))); // A3+T(A5+TA7)
            fp4 = packFloatx80(0, 0x3FF8, LIT64(0x88888888888859AF));
            fp2 = floatx80_add(fp2, fp4); // A2+T(A4+TA6)
            fp1 = floatx80_mul(fp1, fp3); // T(A3+T(A5+TA7))
            fp2 = floatx80_mul(fp2, fp0); // S(A2+T(A4+TA6))
            fp4 = packFloatx80(1, 0x3FFC, LIT64(0xAAAAAAAAAAAAAA99));
            fp1 = floatx80_add(fp1, fp4); // A1+T(A3+T(A5+TA7))
            fp1 = floatx80_add(fp1, fp2); // [A1+T(A3+T(A5+TA7))]+[S(A2+T(A4+TA6))]
            
            x = packFloatx80(xSign, xExp, xSig);
            fp0 = floatx80_mul(fp0, x); // R'*S
            fp0 = floatx80_mul(fp0, fp1); // SIN(R')-R'
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_add(fp0, x);
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    }
}

/*----------------------------------------------------------------------------
 | Hyperbolic sine
 *----------------------------------------------------------------------------*/

floatx80 floatx80_sinh(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact;
    floatx80 fp0, fp1, fp2;
    float32 fact;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        return packFloatx80(aSign, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);

    if (compact > 0x400CB167) {
        // SINHBIG
        if (compact > 0x400CB2B3) {
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            return roundAndPackFloatx80(floatx80_rounding_precision, aSign, 0x8000, aSig, 0);
        } else {
            fp0 = floatx80_abs(a); // Y = |X|
            fp0 = floatx80_sub(fp0, float64_to_floatx80(LIT64(0x40C62D38D3D64634))); // (|X|-16381LOG2_LEAD)
            fp0 = floatx80_sub(fp0, float64_to_floatx80(LIT64(0x3D6F90AEB1E75CC7))); // |X| - 16381 LOG2, ACCURATE
            fp0 = floatx80_etox(fp0);
            fp2 = packFloatx80(aSign, 0x7FFB, one_sig);
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_mul(fp0, fp2);
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    } else { // |X| < 16380 LOG2
        fp0 = floatx80_abs(a); // Y = |X|
        fp0 = floatx80_etoxm1(fp0); // FP0 IS Z = EXPM1(Y)
        fp1 = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // 1+Z
        fp2 = fp0;
        fp0 = floatx80_div(fp0, fp1); // Z/(1+Z)
        fp0 = floatx80_add(fp0, fp2);
        
        fact = 0x3F000000;
        fact |= aSign ? 0x80000000 : 0x00000000;
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_mul(fp0, float32_to_floatx80(fact));
        
        float_raise(float_flag_inexact);
        
        return a;
    }
}

/*----------------------------------------------------------------------------
 | Tangent
 *----------------------------------------------------------------------------*/

floatx80 floatx80_tan(floatx80 a)
{
    flag aSign, xSign;
    int32 aExp, xExp;
    bits64 aSig, xSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, l, n, j;
    floatx80 fp0, fp1, fp2, fp3, fp4, fp5, invtwopi, twopi1, twopi2;
    float32 twoto63;
    flag endflag;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        float_raise(float_flag_invalid);
        a.low = floatx80_default_nan_low;
        a.high = floatx80_default_nan_high;
        return a;
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    fp0 = a;
    
    if (compact < 0x3FD78000 || compact > 0x4004BC7E) { // 2^(-40) > |X| > 15 PI
        if (compact > 0x3FFF8000) { // |X| >= 15 PI
            // REDUCEX
            fp1 = packFloatx80(0, 0, 0);
            if (compact == 0x7FFEFFFF) {
                twopi1 = packFloatx80(aSign ^ 1, 0x7FFE, LIT64(0xC90FDAA200000000));
                twopi2 = packFloatx80(aSign ^ 1, 0x7FDC, LIT64(0x85A308D300000000));
                fp0 = floatx80_add(fp0, twopi1);
                fp1 = fp0;
                fp0 = floatx80_add(fp0, twopi2);
                fp1 = floatx80_sub(fp1, fp0);
                fp1 = floatx80_add(fp1, twopi2);
            }
        loop:
            xSign = extractFloatx80Sign(fp0);
            xExp = extractFloatx80Exp(fp0);
            xExp -= 0x3FFF;
            if (xExp <= 28) {
                l = 0;
                endflag = 1;
            } else {
                l = xExp - 27;
                endflag = 0;
            }
            invtwopi = packFloatx80(0, 0x3FFE - l, LIT64(0xA2F9836E4E44152A)); // INVTWOPI
            twopi1 = packFloatx80(0, 0x3FFF + l, LIT64(0xC90FDAA200000000));
            twopi2 = packFloatx80(0, 0x3FDD + l, LIT64(0x85A308D300000000));
            
            twoto63 = 0x5F000000;
            twoto63 |= xSign ? 0x80000000 : 0x00000000; // SIGN(INARG)*2^63 IN SGL
            
            fp2 = floatx80_mul(fp0, invtwopi);
            fp2 = floatx80_add(fp2, float32_to_floatx80(twoto63)); // THE FRACTIONAL PART OF FP2 IS ROUNDED
            fp2 = floatx80_sub(fp2, float32_to_floatx80(twoto63)); // FP2 is N
            fp4 = floatx80_mul(twopi1, fp2); // W = N*P1
            fp5 = floatx80_mul(twopi2, fp2); // w = N*P2
            fp3 = floatx80_add(fp4, fp5); // FP3 is P
            fp4 = floatx80_sub(fp4, fp3); // W-P
            fp0 = floatx80_sub(fp0, fp3); // FP0 is A := R - P
            fp4 = floatx80_add(fp4, fp5); // FP4 is p = (W-P)+w
            fp3 = fp0; // FP3 is A
            fp1 = floatx80_sub(fp1, fp4); // FP1 is a := r - p
            fp0 = floatx80_add(fp0, fp1); // FP0 is R := A+a
            
            if (endflag > 0) {
                n = floatx80_to_int32(fp2);
                goto tancont;
            }
            fp3 = floatx80_sub(fp3, fp0); // A-R
            fp1 = floatx80_add(fp1, fp3); // FP1 is r := (A-R)+a
            goto loop;
        } else {
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_move(a);
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    } else {
        fp1 = floatx80_mul(fp0, float64_to_floatx80(LIT64(0x3FE45F306DC9C883))); // X*2/PI
        
        n = floatx80_to_int32(fp1);
        j = 32 + n;
        
        fp0 = floatx80_sub(fp0, pi_tbl[j]); // X-Y1
        fp0 = floatx80_sub(fp0, float32_to_floatx80(pi_tbl2[j])); // FP0 IS R = (X-Y1)-Y2
        
    tancont:
        if (n & 1) {
            // NODD
            fp1 = fp0; // R
            fp0 = floatx80_mul(fp0, fp0); // S = R*R
            fp3 = float64_to_floatx80(LIT64(0x3EA0B759F50F8688)); // Q4
            fp2 = float64_to_floatx80(LIT64(0xBEF2BAA5A8924F04)); // P3
            fp3 = floatx80_mul(fp3, fp0); // SQ4
            fp2 = floatx80_mul(fp2, fp0); // SP3
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBF346F59B39BA65F))); // Q3+SQ4
            fp4 = packFloatx80(0, 0x3FF6, LIT64(0xE073D3FC199C4A00));
            fp2 = floatx80_add(fp2, fp4); // P2+SP3
            fp3 = floatx80_mul(fp3, fp0); // S(Q3+SQ4)
            fp2 = floatx80_mul(fp2, fp0); // S(P2+SP3)
            fp4 = packFloatx80(0, 0x3FF9, LIT64(0xD23CD68415D95FA1));
            fp3 = floatx80_add(fp3, fp4); // Q2+S(Q3+SQ4)
            fp4 = packFloatx80(1, 0x3FFC, LIT64(0x8895A6C5FB423BCA));
            fp2 = floatx80_add(fp2, fp4); // P1+S(P2+SP3)
            fp3 = floatx80_mul(fp3, fp0); // S(Q2+S(Q3+SQ4))
            fp2 = floatx80_mul(fp2, fp0); // S(P1+S(P2+SP3))
            fp4 = packFloatx80(1, 0x3FFD, LIT64(0xEEF57E0DA84BC8CE));
            fp3 = floatx80_add(fp3, fp4); // Q1+S(Q2+S(Q3+SQ4))
            fp2 = floatx80_mul(fp2, fp1); // RS(P1+S(P2+SP3))
            fp0 = floatx80_mul(fp0, fp3); // S(Q1+S(Q2+S(Q3+SQ4)))
            fp1 = floatx80_add(fp1, fp2); // R+RS(P1+S(P2+SP3))
            fp0 = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // 1+S(Q1+S(Q2+S(Q3+SQ4)))
            
            xSign = extractFloatx80Sign(fp1);
            xExp = extractFloatx80Exp(fp1);
            xSig = extractFloatx80Frac(fp1);
            xSign ^= 1;
            fp1 = packFloatx80(xSign, xExp, xSig);

            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_div(fp0, fp1);
            
            float_raise(float_flag_inexact);
            
            return a;
        } else {
            fp1 = floatx80_mul(fp0, fp0); // S = R*R
            fp3 = float64_to_floatx80(LIT64(0x3EA0B759F50F8688)); // Q4
            fp2 = float64_to_floatx80(LIT64(0xBEF2BAA5A8924F04)); // P3
            fp3 = floatx80_mul(fp3, fp1); // SQ4
            fp2 = floatx80_mul(fp2, fp1); // SP3
            fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0xBF346F59B39BA65F))); // Q3+SQ4
            fp4 = packFloatx80(0, 0x3FF6, LIT64(0xE073D3FC199C4A00));
            fp2 = floatx80_add(fp2, fp4); // P2+SP3
            fp3 = floatx80_mul(fp3, fp1); // S(Q3+SQ4)
            fp2 = floatx80_mul(fp2, fp1); // S(P2+SP3)
            fp4 = packFloatx80(0, 0x3FF9, LIT64(0xD23CD68415D95FA1));
            fp3 = floatx80_add(fp3, fp4); // Q2+S(Q3+SQ4)
            fp4 = packFloatx80(1, 0x3FFC, LIT64(0x8895A6C5FB423BCA));
            fp2 = floatx80_add(fp2, fp4); // P1+S(P2+SP3)
            fp3 = floatx80_mul(fp3, fp1); // S(Q2+S(Q3+SQ4))
            fp2 = floatx80_mul(fp2, fp1); // S(P1+S(P2+SP3))
            fp4 = packFloatx80(1, 0x3FFD, LIT64(0xEEF57E0DA84BC8CE));
            fp3 = floatx80_add(fp3, fp4); // Q1+S(Q2+S(Q3+SQ4))
            fp2 = floatx80_mul(fp2, fp0); // RS(P1+S(P2+SP3))
            fp1 = floatx80_mul(fp1, fp3); // S(Q1+S(Q2+S(Q3+SQ4)))
            fp0 = floatx80_add(fp0, fp2); // R+RS(P1+S(P2+SP3))
            fp1 = floatx80_add(fp1, float32_to_floatx80(0x3F800000)); // 1+S(Q1+S(Q2+S(Q3+SQ4)))
            
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_div(fp0, fp1);
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    }
}

/*----------------------------------------------------------------------------
 | Hyperbolic tangent
 *----------------------------------------------------------------------------*/

floatx80 floatx80_tanh(floatx80 a)
{
    flag aSign, vSign;
    int32 aExp, vExp;
    bits64 aSig, vSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact;
    floatx80 fp0, fp1;
    float32 sign;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        return packFloatx80(aSign, one_exp, one_sig);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(aSign, 0, 0);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact < 0x3FD78000 || compact > 0x3FFFDDCE) {
        // TANHBORS
        if (compact < 0x3FFF8000) {
            // TANHSM
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_move(a);
            
            float_raise(float_flag_inexact);
            
            return a;
        } else {
            if (compact > 0x40048AA1) {
                // TANHHUGE
                sign = 0x3F800000;
                sign |= aSign ? 0x80000000 : 0x00000000;
                fp0 = float32_to_floatx80(sign);
                sign &= 0x80000000;
                sign ^= 0x80800000; // -SIGN(X)*EPS
                
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                a = floatx80_add(fp0, float32_to_floatx80(sign));
                
                float_raise(float_flag_inexact);
                
                return a;
            } else {
                fp0 = packFloatx80(0, aExp+1, aSig); // Y = 2|X|
                fp0 = floatx80_etox(fp0); // FP0 IS EXP(Y)
                fp0 = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // EXP(Y)+1
                sign = aSign ? 0x80000000 : 0x00000000;
                fp1 = floatx80_div(float32_to_floatx80(sign^0xC0000000), fp0); // -SIGN(X)*2 / [EXP(Y)+1]
                fp0 = float32_to_floatx80(sign | 0x3F800000); // SIGN
                
                float_rounding_mode = user_rnd_mode;
                floatx80_rounding_precision = user_rnd_prec;
                
                a = floatx80_add(fp1, fp0);
                
                float_raise(float_flag_inexact);
                
                return a;
            }
        }
    } else { // 2**(-40) < |X| < (5/2)LOG2
        fp0 = packFloatx80(0, aExp+1, aSig); // Y = 2|X|
        fp0 = floatx80_etoxm1(fp0); // FP0 IS Z = EXPM1(Y)
        fp1 = floatx80_add(fp0, float32_to_floatx80(0x40000000)); // Z+2
        
        vSign = extractFloatx80Sign(fp1);
        vExp = extractFloatx80Exp(fp1);
        vSig = extractFloatx80Frac(fp1);
        
        fp1 = packFloatx80(vSign ^ aSign, vExp, vSig);
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_div(fp0, fp1);
        
        float_raise(float_flag_inexact);
        
        return a;
    }
}

/*----------------------------------------------------------------------------
 | 10 to x
 *----------------------------------------------------------------------------*/

floatx80 floatx80_tentox(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, n, j, l, m, m1;
    floatx80 fp0, fp1, fp2, fp3, adjfact, fact1, fact2;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        if (aSign) return packFloatx80(0, 0, 0);
        return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(0, one_exp, one_sig);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    fp0 = a;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact < 0x3FB98000 || compact > 0x400B9B07) { // |X| > 16480 LOG2/LOG10 or |X| < 2^(-70)
        if (compact > 0x3FFF8000) { // |X| > 16480
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            if (aSign) {
                return roundAndPackFloatx80(floatx80_rounding_precision, 0, -0x1000, aSig, 0);
            } else {
                return roundAndPackFloatx80(floatx80_rounding_precision, 0, 0x8000, aSig, 0);
            }
        } else { // |X| < 2^(-70)
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // 1 + X
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    } else { // 2^(-70) <= |X| <= 16480 LOG 2 / LOG 10
        fp1 = fp0; // X
        fp1 = floatx80_mul(fp1, float64_to_floatx80(LIT64(0x406A934F0979A371))); // X*64*LOG10/LOG2
        n = floatx80_to_int32(fp1); // N=INT(X*64*LOG10/LOG2)
        fp1 = int32_to_floatx80(n);

        j = n & 0x3F;
        l = n / 64; // NOTE: this is really arithmetic right shift by 6
        if (n < 0 && j) { // arithmetic right shift is division and round towards minus infinity
            l--;
        }
        m = l / 2; // NOTE: this is really arithmetic right shift by 1
        if (l < 0 && (l & 1)) { // arithmetic right shift is division and round towards minus infinity
            m--;
        }
        m1 = l - m;
        m1 += 0x3FFF; // ADJFACT IS 2^(M')

        adjfact = packFloatx80(0, m1, one_sig);
        fact1 = exp2_tbl[j];
        fact1.high += m;
        fact2.high = exp2_tbl2[j]>>16;
        fact2.high += m;
        fact2.low = (bits64)(exp2_tbl2[j] & 0xFFFF);
        fact2.low <<= 48;
        
        fp2 = fp1; // N
        fp1 = floatx80_mul(fp1, float64_to_floatx80(LIT64(0x3F734413509F8000))); // N*(LOG2/64LOG10)_LEAD
        fp3 = packFloatx80(1, 0x3FCD, LIT64(0xC0219DC1DA994FD2));
        fp2 = floatx80_mul(fp2, fp3); // N*(LOG2/64LOG10)_TRAIL
        fp0 = floatx80_sub(fp0, fp1); // X - N L_LEAD
        fp0 = floatx80_sub(fp0, fp2); // X - N L_TRAIL
        fp2 = packFloatx80(0, 0x4000, LIT64(0x935D8DDDAAA8AC17)); // LOG10
        fp0 = floatx80_mul(fp0, fp2); // R
        
        // EXPR
        fp1 = floatx80_mul(fp0, fp0); // S = R*R
        fp2 = float64_to_floatx80(LIT64(0x3F56C16D6F7BD0B2)); // A5
        fp3 = float64_to_floatx80(LIT64(0x3F811112302C712C)); // A4
        fp2 = floatx80_mul(fp2, fp1); // S*A5
        fp3 = floatx80_mul(fp3, fp1); // S*A4
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FA5555555554CC1))); // A3+S*A5
        fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0x3FC5555555554A54))); // A2+S*A4
        fp2 = floatx80_mul(fp2, fp1); // S*(A3+S*A5)
        fp3 = floatx80_mul(fp3, fp1); // S*(A2+S*A4)
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FE0000000000000))); // A1+S*(A3+S*A5)
        fp3 = floatx80_mul(fp3, fp0); // R*S*(A2+S*A4)
        
        fp2 = floatx80_mul(fp2, fp1); // S*(A1+S*(A3+S*A5))
        fp0 = floatx80_add(fp0, fp3); // R+R*S*(A2+S*A4)
        fp0 = floatx80_add(fp0, fp2); // EXP(R) - 1
        
        fp0 = floatx80_mul(fp0, fact1);
        fp0 = floatx80_add(fp0, fact2);
        fp0 = floatx80_add(fp0, fact1);
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_mul(fp0, adjfact);
        
        float_raise(float_flag_inexact);
        
        return a;
    }
}

/*----------------------------------------------------------------------------
 | 2 to x
 *----------------------------------------------------------------------------*/

floatx80 floatx80_twotox(floatx80 a)
{
    flag aSign;
    int32 aExp;
    bits64 aSig;
    
    int8 user_rnd_mode, user_rnd_prec;
    
    int32 compact, n, j, l, m, m1;
    floatx80 fp0, fp1, fp2, fp3, adjfact, fact1, fact2;
    
    aSig = extractFloatx80Frac(a);
    aExp = extractFloatx80Exp(a);
    aSign = extractFloatx80Sign(a);
    
    if (aExp == 0x7FFF) {
        if ((bits64) (aSig<<1)) return propagateFloatx80NaNOneArg(a);
        if (aSign) return packFloatx80(0, 0, 0);
        return packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
    }
    
    if (aExp == 0 && aSig == 0) {
        return packFloatx80(0, one_exp, one_sig);
    }
    
    user_rnd_mode = float_rounding_mode;
    user_rnd_prec = floatx80_rounding_precision;
    float_rounding_mode = float_round_nearest_even;
    floatx80_rounding_precision = 80;
    
    fp0 = a;
    
    compact = floatx80_make_compact(aExp, aSig);
    
    if (compact < 0x3FB98000 || compact > 0x400D80C0) { // |X| > 16480 or |X| < 2^(-70)
        if (compact > 0x3FFF8000) { // |X| > 16480
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            if (aSign) {
                return roundAndPackFloatx80(floatx80_rounding_precision, 0, -0x1000, aSig, 0);
            } else {
                return roundAndPackFloatx80(floatx80_rounding_precision, 0, 0x8000, aSig, 0);
            }
        } else { // |X| < 2^(-70)
            float_rounding_mode = user_rnd_mode;
            floatx80_rounding_precision = user_rnd_prec;
            
            a = floatx80_add(fp0, float32_to_floatx80(0x3F800000)); // 1 + X
            
            float_raise(float_flag_inexact);
            
            return a;
        }
    } else { // 2^(-70) <= |X| <= 16480
        fp1 = fp0; // X
        fp1 = floatx80_mul(fp1, float32_to_floatx80(0x42800000)); // X * 64
        n = floatx80_to_int32(fp1);
        fp1 = int32_to_floatx80(n);
        j = n & 0x3F;
        l = n / 64; // NOTE: this is really arithmetic right shift by 6
        if (n < 0 && j) { // arithmetic right shift is division and round towards minus infinity
            l--;
        }
        m = l / 2; // NOTE: this is really arithmetic right shift by 1
        if (l < 0 && (l & 1)) { // arithmetic right shift is division and round towards minus infinity
            m--;
        }
        m1 = l - m;
        m1 += 0x3FFF; // ADJFACT IS 2^(M')
        
        adjfact = packFloatx80(0, m1, one_sig);
        fact1 = exp2_tbl[j];
        fact1.high += m;
        fact2.high = exp2_tbl2[j]>>16;
        fact2.high += m;
        fact2.low = (bits64)(exp2_tbl2[j] & 0xFFFF);
        fact2.low <<= 48;
        
        fp1 = floatx80_mul(fp1, float32_to_floatx80(0x3C800000)); // (1/64)*N
        fp0 = floatx80_sub(fp0, fp1); // X - (1/64)*INT(64 X)
        fp2 = packFloatx80(0, 0x3FFE, LIT64(0xB17217F7D1CF79AC)); // LOG2
        fp0 = floatx80_mul(fp0, fp2); // R
        
        // EXPR
        fp1 = floatx80_mul(fp0, fp0); // S = R*R
        fp2 = float64_to_floatx80(LIT64(0x3F56C16D6F7BD0B2)); // A5
        fp3 = float64_to_floatx80(LIT64(0x3F811112302C712C)); // A4
        fp2 = floatx80_mul(fp2, fp1); // S*A5
        fp3 = floatx80_mul(fp3, fp1); // S*A4
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FA5555555554CC1))); // A3+S*A5
        fp3 = floatx80_add(fp3, float64_to_floatx80(LIT64(0x3FC5555555554A54))); // A2+S*A4
        fp2 = floatx80_mul(fp2, fp1); // S*(A3+S*A5)
        fp3 = floatx80_mul(fp3, fp1); // S*(A2+S*A4)
        fp2 = floatx80_add(fp2, float64_to_floatx80(LIT64(0x3FE0000000000000))); // A1+S*(A3+S*A5)
        fp3 = floatx80_mul(fp3, fp0); // R*S*(A2+S*A4)
        
        fp2 = floatx80_mul(fp2, fp1); // S*(A1+S*(A3+S*A5))
        fp0 = floatx80_add(fp0, fp3); // R+R*S*(A2+S*A4)
        fp0 = floatx80_add(fp0, fp2); // EXP(R) - 1
        
        fp0 = floatx80_mul(fp0, fact1);
        fp0 = floatx80_add(fp0, fact2);
        fp0 = floatx80_add(fp0, fact1);
        
        float_rounding_mode = user_rnd_mode;
        floatx80_rounding_precision = user_rnd_prec;
        
        a = floatx80_mul(fp0, adjfact);
        
        float_raise(float_flag_inexact);
        
        return a;
    }
}
