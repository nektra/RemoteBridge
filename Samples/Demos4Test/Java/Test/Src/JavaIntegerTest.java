/*
 * Copyright (C) 2010-2015 Nektra S.A., Buenos Aires, Argentina.
 * All rights reserved. Contact: http://www.nektra.com
 *
 *
 * This file is part of Remote Bridge
 *
 *
 * Commercial License Usage
 * ------------------------
 * Licensees holding valid commercial Remote Bridge licenses may use this
 * file in accordance with the commercial license agreement provided with
 * the Software or, alternatively, in accordance with the terms contained
 * in a written agreement between you and Nektra.  For licensing terms and
 * conditions see http://www.nektra.com/licensing/. Use the contact form
 * at http://www.nektra.com/contact/ for further information.
 *
 *
 * GNU General Public License Usage
 * --------------------------------
 * Alternatively, this file may be used under the terms of the GNU General
 * Public License version 3.0 as published by the Free Software Foundation
 * and appearing in the file LICENSE.GPL included in the packaging of this
 * file.  Please visit http://www.gnu.org/copyleft/gpl.html and review the
 * information to ensure the GNU General Public License version 3.0
 * requirements will be met.
 *
 **/
class JavaIntegerTest
{
	//--------------------------------------------------------------------------
	//Integer test
	
	public static int Test(int param)
	{
		return 255 - param;
	}

	public static int[][][] Test3D(int[][][] param)
	{
		int i1, i2, i3, len1, len2, len3;

		len1 = param.length;
		for (i1=0; i1<len1; i1++)
		{
			len2 = param[i1].length;
			for (i2=0; i2<len2; i2++)
			{
				len3 = param[i1][i2].length;
				for (i3=0; i3<len3; i3++)
				{
					param[i1][i2][i3] = 255 - param[i1][i2][i3];
				}
			}
		}
		return param;
	}

	//--------------------------------------------------------------------------
	//Integer Object test

	public static Integer TestObj(Integer param)
	{
		return new Integer(255 - param.intValue());
	}

	public static Integer[][][] TestObj3D(Integer[][][] param)
	{
		int i1, i2, i3, len1, len2, len3;

		len1 = param.length;
		for (i1=0; i1<len1; i1++)
		{
			len2 = param[i1].length;
			for (i2=0; i2<len2; i2++)
			{
				len3 = param[i1][i2].length;
				for (i3=0; i3<len3; i3++)
				{
					param[i1][i2][i3] = new Integer(255 - param[i1][i2][i3].intValue()); 
				}
			}
		}
		return param;
	}
}
