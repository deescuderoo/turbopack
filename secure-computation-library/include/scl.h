/**
 * @file scl.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#ifndef _SCL_MAIN_H
#define _SCL_MAIN_H

#include "scl/math.h"
#include "scl/networking.h"
#include "scl/primitives.h"
#include "scl/secret_sharing.h"

/**
 * @brief Primary namespace.
 *
 * This is the top level namespace and is where all high level functionality is
 * placed.
 *
 * <h2>Math</h2>
 * Math related functionality is located in the headers in
 * <code>scl/math/</code>, or alternatively <code>scl/math.h</code>.
 *
 *
 * <h2>Secret Sharing</h2>
 * SCL can perform both additive and Shamir secret-sharing. Include some of the
 * headers in <code>scl/ss/</code> or the <<code>scl/secret_sharing.h</code>
 * header for access.
 *
 * <h2>Networking</h2>
 * SCL provides some limited functionality when it comes to operating with a
 * network and peer-to-peer channels. This functionality can be accessed through
 * <code>scl/networking.h</code> or headers in <code>scl/net/</code>.
 */
namespace scl {

/**
 * @brief Internal details, low level functions and other fun stuff.
 *
 * Danger zone.
 */
namespace details {}
}  // namespace scl

#endif /* _SCL_MAIN_H */
