/* GPLv2 License
 *
 * Copyright (C) 2016-2018 Lixing Ding <ding.lixing@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 **/

#ifndef __TEST_INC__
#define __TEST_INC__

#include <cupkee.h>

#include "hw_mock.h"

#include "CUnit.h"
#include "CUnit_Basic.h"

int TU_pre_init(void);
int TU_pre_deinit(void);
int TU_object_event_dispatch(void);
int TU_pin_event_dispatch(void);

CU_pSuite test_hello(void);

CU_pSuite test_sys_event(void);
CU_pSuite test_sys_memory(void);
CU_pSuite test_sys_timeout(void);
CU_pSuite test_sys_process(void);
CU_pSuite test_sys_struct(void);
CU_pSuite test_sys_stream(void);
CU_pSuite test_sys_object(void);

CU_pSuite test_sys_device(void);
CU_pSuite test_sys_pin(void);
CU_pSuite test_sys_timer(void);

#endif /* __TEST_INC__ */

