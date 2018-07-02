/*
 * Copyright (c) 2016 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */


#ifndef __XI_DRAWING_API_H__
#define __XI_DRAWING_API_H__

#include "xi_core_api.h"

/* Circle drawing functions */
_XI_API_ XI_ERR_TYPE xiCircle_I8(xi_pTile src, xi_point32 c, int r, int color, int thickness);
_XI_API_ XI_ERR_TYPE xiCircle_I8Sa(xi_pTile src, xi_point32 c, int r, int color, int thickness);

_XI_API_ XI_ERR_TYPE xiCircle_I16(xi_pTile src, xi_point32 c, int r, int color, int thickness);
_XI_API_ XI_ERR_TYPE xiCircle_I16Sa(xi_pTile src, xi_point32 c, int r, int color, int thickness);


/* Line drawing functions */
_XI_API_ XI_ERR_TYPE xiLine_I8(xi_pTile src, xi_point32 p1, xi_point32 p2, int color, int thickness);
_XI_API_ XI_ERR_TYPE xiLine_I16(xi_pTile src, xi_point32 p1, xi_point32 p2, int color, int thickness);


/* Rectangle */
_XI_API_ XI_ERR_TYPE xiRectangle_I8(xi_pTile src, xi_point32 p0, xi_point32 p1, int color, int thickness);
_XI_API_ XI_ERR_TYPE xiRectangle_I16(xi_pTile src, xi_point32 p0, xi_point32 p1, int color, int thickness);


#endif
