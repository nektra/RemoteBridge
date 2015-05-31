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
import java.awt.Toolkit;
import java.awt.Dimension;
import javax.swing.*;

public class JavaTest
{
	public static void main(String[] args)
	{
		JFrame frame = new JFrame("JavaTest");
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		frame.setSize(400, 100);
		Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
		int x = (int)((d.getWidth() - frame.getWidth()) / 2);
		int y = (int)((d.getHeight() - frame.getHeight()) / 2);
		frame.setLocation(x, y);
		frame.setVisible(true);
		JLabel label = new JLabel("Test application is running.");
		frame.getContentPane().add(label);
		//JOptionPane.showMessageDialog(null, "Test application started. Press the OK button to exit.", "JavaTest", JOptionPane.INFORMATION_MESSAGE);
	}
}
