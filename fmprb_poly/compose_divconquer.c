/*=============================================================================

    This file is part of ARB.

    ARB is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ARB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ARB; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

=============================================================================*/
/******************************************************************************

    Copyright (C) 2010 William Hart
    Copyright (C) 2012 Sebastian Pancratz
    Copyright (C) 2012 Fredrik Johansson

******************************************************************************/

#include "fmprb_poly.h"

void
_fmprb_poly_compose_divconquer(fmprb_struct * res, const fmprb_struct * poly1, long len1,
                                          const fmprb_struct * poly2, long len2, long prec)
{
    long i, j, k, n;
    long *hlen, alloc, powlen;
    fmprb_struct *v, **h, *pow, *temp;
    
    if (len1 == 1)
    {
        fmprb_set(res, poly1);
        return;
    }
    if (len2 == 1)
    {
        _fmprb_poly_evaluate(res, poly1, len1, poly2, prec);
        return;
    }
    if (len1 == 2)
    {
        _fmprb_poly_compose_horner(res, poly1, len1, poly2, len2, prec);
        return;
    }

    /* Initialisation */
    
    hlen = (long *) flint_malloc(((len1 + 1) / 2) * sizeof(long));
    
    for (k = 1; (2 << k) < len1; k++) ;
    
    hlen[0] = hlen[1] = ((1 << k) - 1) * (len2 - 1) + 1;
    for (i = k - 1; i > 0; i--)
    {
        long hi = (len1 + (1 << i) - 1) / (1 << i);
        for (n = (hi + 1) / 2; n < hi; n++)
            hlen[n] = ((1 << i) - 1) * (len2 - 1) + 1;
    }
    powlen = (1 << k) * (len2 - 1) + 1;
    
    alloc = 0;
    for (i = 0; i < (len1 + 1) / 2; i++)
        alloc += hlen[i];

    v = _fmprb_vec_init(alloc + 2 * powlen);
    h = (fmprb_struct **) flint_malloc(((len1 + 1) / 2) * sizeof(fmprb_struct *));
    h[0] = v;
    for (i = 0; i < (len1 - 1) / 2; i++)
    {
        h[i + 1] = h[i] + hlen[i];
        hlen[i] = 0;
    }
    hlen[(len1 - 1) / 2] = 0;
    pow = v + alloc;
    temp = pow + powlen;
    
    /* Let's start the actual work */
    
    for (i = 0, j = 0; i < len1 / 2; i++, j += 2)
    {
        if (!fmprb_is_zero(poly1 + j + 1))
        {
            _fmprb_vec_scalar_mul(h[i], poly2, len2, poly1 + j + 1, prec);
            fmprb_add(h[i], h[i], poly1 + j, prec);
            hlen[i] = len2;
        }
        else if (!fmprb_is_zero(poly1 + j))
        {
            fmprb_set(h[i], poly1 + j);
            hlen[i] = 1;
        }
    }
    if ((len1 & 1L))
    {
        if (!fmprb_is_zero(poly1 + j))
        {
            fmprb_set(h[i], poly1 + j);
            hlen[i] = 1;
        }
    }
    
    _fmprb_poly_mul(pow, poly2, len2, poly2, len2, prec);
    powlen = 2 * len2 - 1;
    
    for (n = (len1 + 1) / 2; n > 2; n = (n + 1) / 2)
    {
        if (hlen[1] > 0)
        {
            long templen = powlen + hlen[1] - 1;
            _fmprb_poly_mul(temp, pow, powlen, h[1], hlen[1], prec);
            _fmprb_poly_add(h[0], temp, templen, h[0], hlen[0], prec);
            hlen[0] = FLINT_MAX(hlen[0], templen);
        }
        
        for (i = 1; i < n / 2; i++)
        {
            if (hlen[2*i + 1] > 0)
            {
                _fmprb_poly_mul(h[i], pow, powlen, h[2*i + 1], hlen[2*i + 1], prec);
                hlen[i] = hlen[2*i + 1] + powlen - 1;
            } else
                hlen[i] = 0;
            _fmprb_poly_add(h[i], h[i], hlen[i], h[2*i], hlen[2*i], prec);
            hlen[i] = FLINT_MAX(hlen[i], hlen[2*i]);
        }
        if ((n & 1L))
        {
            _fmprb_vec_set(h[i], h[2*i], hlen[2*i]);
            hlen[i] = hlen[2*i];
        }
        
        _fmprb_poly_mul(temp, pow, powlen, pow, powlen, prec);
        powlen += powlen - 1;
        {
            fmprb_struct * t = pow;
            pow = temp;
            temp = t;
        }
    }

    _fmprb_poly_mul(res, pow, powlen, h[1], hlen[1], prec);
    _fmprb_vec_add(res, res, h[0], hlen[0], prec);
    
    _fmprb_vec_clear(v, alloc + 2 * powlen);
    flint_free(h);
    flint_free(hlen);
}

void
fmprb_poly_compose_divconquer(fmprb_poly_t res,
                             const fmprb_poly_t poly1, const fmprb_poly_t poly2, long prec)
{
    const long len1 = poly1->length;
    const long len2 = poly2->length;
    
    if (len1 == 0)
    {
        fmprb_poly_zero(res);
    }
    else if (len1 == 1 || len2 == 0)
    {
        fmprb_poly_set_fmprb(res, poly1->coeffs);
    }
    else
    {
        const long lenr = (len1 - 1) * (len2 - 1) + 1;
        
        if (res != poly1 && res != poly2)
        {
            fmprb_poly_fit_length(res, lenr);
            _fmprb_poly_compose_divconquer(res->coeffs, poly1->coeffs, len1,
                                                   poly2->coeffs, len2, prec);
        }
        else
        {
            fmprb_poly_t t;
            fmprb_poly_init2(t, lenr);
            _fmprb_poly_compose_divconquer(t->coeffs, poly1->coeffs, len1,
                                                 poly2->coeffs, len2, prec);
            fmprb_poly_swap(res, t);
            fmprb_poly_clear(t);
        }

        _fmprb_poly_set_length(res, lenr);
        _fmprb_poly_normalise(res);
    }
}