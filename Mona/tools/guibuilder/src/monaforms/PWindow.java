package guibuilder;

import java.awt.*;
import java.awt.event.*;
import java.io.*;

/**
 �E�B���h�E���i
 */
public class PWindow extends PBase {
	/** ����{�^�� (�p���b�g) */
	static int icon_palette[] = {
		0xff1c1a1c,
		0xff8c8e8c,
		0xffcccecc,
		0xffacaeac,
		0xffeceeec,
		0xff9c9e9c,
		0xffdcdedc,
		0xffbcbebc,
		0xfffcfefc,
	};

	/** ����{�^�� (�f�[�^) */
	static byte icon_data[] = {
		0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x2,
		0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8,
		0x1,0x0,0x8,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x8,
		0x1,0x0,0x2,0x5,0x5,0x3,0x3,0x7,0x7,0x2,0x1,0x0,0x8,
		0x1,0x0,0x2,0x5,0x3,0x3,0x7,0x7,0x2,0x2,0x1,0x0,0x8,
		0x1,0x0,0x2,0x3,0x3,0x7,0x7,0x2,0x2,0x6,0x1,0x0,0x8,
		0x1,0x0,0x2,0x3,0x7,0x7,0x2,0x2,0x6,0x6,0x1,0x0,0x8,
		0x1,0x0,0x2,0x7,0x7,0x2,0x2,0x6,0x6,0x4,0x1,0x0,0x8,
		0x1,0x0,0x2,0x7,0x2,0x2,0x6,0x6,0x4,0x4,0x1,0x0,0x8,
		0x1,0x0,0x2,0x2,0x2,0x6,0x6,0x4,0x4,0x8,0x1,0x0,0x8,
		0x1,0x0,0x2,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x8,
		0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8,
		0x2,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	};
	
	/** ���i�ʂ��ԍ� */
	private static int count = 0;
	
	/** �R���X�g���N�^ */
	public PWindow() {
	}
	
	/** �`��n���h�� */
	public void paint(Graphics g) {
		int i, j;
		int w = getWidth();
		int h = getHeight();
		int oy = INSETS_TOP;
		
		FontMetrics metrics = getFontMetrics(getFont());
		int fw = metrics.stringWidth(getCaption());
		int fh = metrics.getAscent() - metrics.getDescent();
		int tx = 2 + 2 + (oy - 8) + 2;
		int tw = tx + fw + 1 + 2 + 2;
		
		// �^�C�g���o�[
		DrawEngraved(g, 0, 0, tw, oy - 2);
		g.setColor(new Color(0xffffe000, true));
		g.fillRect(2, 2, tw - 4, oy - 4);
		
		// ����{�^��
		DrawEngraved(g, 4, 4, oy - 8, oy - 8);
		
		// �O�g
		g.setColor(getBackground());
		g.fillRect(0, oy - 2, w, h - (oy - 2));
		DrawEngraved(g, 0, oy - 2, w, h - (oy - 2));
		
		// �L���v�V����
		g.drawString(getCaption(), tx, 4 + fh);
		g.drawString(getCaption(), tx + 1, 4 + fh);
		
		paintComponents(g);
	}
	
	//
	// IParts
	//
	
	/** ���i�쐬���ɌĂ΂�� */
	public void create() {
		setBounds(100, 100, 200, 200);
		setForeground(DEFAULT_FORECOLOR);
		setBackground(DEFAULT_BACKCOLOR);
		setLayout(null);
		enableEvents(AWTEvent.MOUSE_EVENT_MASK);
		setName("Form" + (count + 1));
		setCaption("Form" + (count + 1));
		setFontStyle("FONT_PLAIN");
		count++;
	}
	
	/** ���i�폜���ɌĂ΂�� */
	public void dispose() {
		this.count = 0;
	}
	
	/** �}�E�X�C�x���g */
	public void processMouseEvent(MouseEvent e) {
		if (e.getID() == MouseEvent.MOUSE_PRESSED) {
			System.out.println("PWindow: source = " + e.getSource());
			((ApplicationWindow)getParent()).update(this);
		}
		super.processMouseEvent(e);
	}
}