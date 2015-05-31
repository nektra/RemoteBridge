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
import java.lang.*;

class InjectTestWithCallbacks
{
	//--------------------------------------------------------------------------
	//Injected class with native callbacks test

	public static native int noParametersReturningInt();
	public static native Object noParametersReturningJavaLangDoubleObject();
	public static native int withParametersReturningInt(String strParam, Object objParam, int[][] intArrayParam, double dblValue, Float[] floatObjArrayParam);
	public static native Object withParametersReturningJavaLangDoubleObject(String strParam, Object objParam, int[][] intArrayParam, double dblValue, Float[] floatObjArrayParam);

	public int Test_NoParametersReturningInt()
	{
		return noParametersReturningInt();
	}

	public Object Test_NoParametersReturningJavaLangDoubleObject()
	{
		return noParametersReturningJavaLangDoubleObject();
	}

	public int Test_WithParametersReturningInt()
	{
		Float[] flt = new Float[2];
		flt[0] = new Float(10.0);
		flt[1] = new Float(20.0);
		int[][] intArray = new int[2][3];
		for (int i=0; i<2; i++)
		{
			for (int j=0; j<3; j++)
				intArray[i][j] = i+j;
		}
		return withParametersReturningInt("Test_WithParametersReturningInt", new Integer(35), intArray, 50.0, flt);
	}

	public Object Test_WithParametersReturningJavaLangDoubleObject()
	{
		Float[] flt = new Float[2];
		flt[0] = new Float(10.0);
		flt[1] = new Float(20.0);
		int[][] intArray = new int[2][3];
		for (int i=0; i<2; i++)
		{
			for (int j=0; j<3; j++)
				intArray[i][j] = i+j;
		}
		return withParametersReturningJavaLangDoubleObject("Test_WithParametersReturningJavaLangDoubleObject", new Integer(35), intArray, 50.0, flt);
	}
}
