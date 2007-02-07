/* callbacks_IO.c - this file is part of DeSmuME
 *
 * Copyright (C) 2007 Damien Nozay (damdoum)
 * Copyright (C) 2007 Pascal Giard (evilynux)
 * Author: damdoum at users.sourceforge.net
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "callbacks_IO.h"

static u16 Cur_Keypad = 0;
float ScreenCoeff_Size[2]={1.0,1.0};
gboolean ScreenRotate=FALSE;
gboolean Boost=FALSE;
int BoostFS=20;
int saveFS;

/* ***** ***** INPUT BUTTONS / KEYBOARD ***** ***** */
gboolean  on_wMainW_key_press_event    (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	u16 Key = lookup_key(event->keyval);
	if (event->keyval == keyboard_cfg[KEY_BOOST-1]) {
		Boost = !Boost;
		if (Boost) {
			saveFS = Frameskip;
			Frameskip = BoostFS;
		} else {
			Frameskip = saveFS;
		}
	}
        ADD_KEY( Cur_Keypad, Key );
        if(desmume_running()) update_keypad(Cur_Keypad);
	return 1;
}

gboolean  on_wMainW_key_release_event  (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	u16 Key = lookup_key(event->keyval);
        RM_KEY( Cur_Keypad, Key );
        if(desmume_running()) update_keypad(Cur_Keypad);
	return 1;
}



/* ***** ***** SCREEN DRAWING ***** ***** */
int has_pix_col_map=0;
u32 pix_col_map[0x8000];

void init_pix_col_map() {
	/* precalc colors so we get some fps */
	int a,b,c,A,B,C,rA,rB,rC;
	if (has_pix_col_map) return;
	for (a=0; a<0x20; a++) {
		A=a<<10; rA=A<<9;
		for (b=0; b<0x20; b++) {
			B=b<<5; rB=B<<6;
			for (c=0; c<0x20; c++) {
				C=c; rC=C<<3;
				pix_col_map[A|B|C]=rA|rB|rC;
			}
		}
	}
	has_pix_col_map=1;
}

#define RAW_W 256
#define RAW_H 192
#define RAW_OFFSET 256*192*sizeof(u16)
#define MAX_SIZE 3
u32 on_screen_image32[RAW_W*RAW_H*2*MAX_SIZE*MAX_SIZE];

int inline screen_size() {
	int sz = ScreenCoeff_Size[0];
	return RAW_W*RAW_H*2*sz*sz*sizeof(u32);
}
int inline offset_pixels_lower_screen() {
	return screen_size()/2;
}

void black_screen () {
	/* removes artifacts when resizing with scanlines */
	memset(on_screen_image32,0,screen_size());
}

void decode_screen () {

	int x,y, m, W,H,L,BL;
	u32 image[RAW_H*2][RAW_W], pix;
	u16 * pixel = (u16*)&GPU_screen;
	u32 * rgb32 = &on_screen_image32[0];

	/* decode colors */
	init_pix_col_map();
	for (y=0; y<RAW_H*2; y++) {
		for (x=0; x<RAW_W; x++) {
			image[y][x] = pix_col_map[*pixel&0x07FFF];
			pixel++;
		}
	}
	#define LOOP(a,b,c,d,e,f) \
		L=W*ScreenCoeff_Size[0]; \
		BL=L*sizeof(u32); \
		for (a; b; c) { \
			for (d; e; f) { \
				pix = image[y][x]; \
				for (m=0; m<ScreenCoeff_Size[0]; m++) { \
					*rgb32 = pix; rgb32++; \
				} \
			} \
			/* lines duplicated for scaling height */ \
			for (m=1; m<ScreenCoeff_Size[0]; m++) { \
				memmove(rgb32, rgb32-L, BL); \
				rgb32 += L; \
			} \
		}
	/* load pixels in buffer accordingly */
	if (ScreenRotate) {
		W=RAW_H; H=RAW_W;
		LOOP(x=RAW_W-1, x >= 0, x--, y=0, y < RAW_H, y++)
		LOOP(x=RAW_W-1, x >= 0, x--, y=RAW_H, y < RAW_H*2, y++)
	} else {
		H=RAW_H*2; W=RAW_W;
		LOOP(y=0, y < RAW_H*2, y++, x=0, x < RAW_W, x++)
	}
}

#ifndef HAVE_LIBGDKGLEXT_X11_1_0
// they are empty if no opengl
// else see gdk_gl.c / gdk_gl.h
BOOL my_gl_Begin (int screen) { return FALSE; }
void my_gl_End (int screen) {}
void init_GL_capabilities() {}
void init_GL(GtkWidget * widget, int screen) {}
void reshape (GtkWidget * widget, int screen) {}

gboolean screen (GtkWidget * widget, int off) {
	int H,W,L;
	if (off==0)
		decode_screen();

	if (ScreenRotate) {
		W=RAW_H; H=RAW_W;
	} else {
		H=RAW_H; W=RAW_W;
	}
	L=W*ScreenCoeff_Size[0]*sizeof(u32);
	off*= offset_pixels_lower_screen();

	gdk_draw_rgb_32_image	(widget->window,
		widget->style->fg_gc[widget->state],0,0, 
		W*ScreenCoeff_Size[0], H*ScreenCoeff_Size[0],
		GDK_RGB_DITHER_NONE,((guchar*)on_screen_image32)+off,L);
	return TRUE;
}
#endif /* if HAVE_LIBGDKGLEXT_X11_1_0 */


/* OUTPUT UPPER SCREEN  */
/* OUTPUT LOWER SCREEN  */
void      on_wDraw_Main_realize       (GtkWidget *widget, gpointer user_data) {
	init_GL(widget, 0);
}
void      on_wDraw_Sub_realize        (GtkWidget *widget, gpointer user_data) {
	init_GL(widget, 1);
}

gboolean  on_wDraw_Main_expose_event  (GtkWidget *widget, GdkEventExpose  *event, gpointer user_data) {
	return screen(widget, 0);
}
gboolean  on_wDraw_Sub_expose_event   (GtkWidget *widget, GdkEventExpose  *event, gpointer user_data) {
	return screen(widget, 1);
}

gboolean  on_wDraw_Main_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
	reshape(widget, 0); return TRUE;
}
gboolean  on_wDraw_Sub_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
	reshape(widget, 1); return TRUE;
}










/* ***** ***** INPUT STYLUS / MOUSE ***** ***** */

void set_touch_pos (int x, int y) {
	s32 EmuX, EmuY;
	x /= ScreenCoeff_Size[1];
	y /= ScreenCoeff_Size[1];
	EmuX = x; EmuY = y;
	if (ScreenRotate) { EmuX = 256-y; EmuY = x; }
	if(EmuX<0) EmuX = 0; else if(EmuX>255) EmuX = 255;
	if(EmuY<0) EmuY = 0; else if(EmuY>192) EmuY = 192;
	NDS_setTouchPos(EmuX, EmuY);
}

gboolean  on_wDraw_Main_button_press_event   (GtkWidget *widget, GdkEventButton  *event, gpointer user_data) {
	switch (event->button) {
	case 1: break;
	case 3: break;
	case 2: ScreenCoeff_Size[0]=1.0;
		resize(ScreenCoeff_Size[0],ScreenCoeff_Size[1]); break;
	}
	return TRUE;
}

gboolean  on_wDraw_Main_button_release_event (GtkWidget *widget, GdkEventButton  *event, gpointer user_data) {
	return TRUE;
}

gboolean  on_wDraw_Sub_button_press_event   (GtkWidget *widget, GdkEventButton  *event, gpointer user_data) {
	GdkModifierType state;
	gint x,y;
	
	switch (event->button) {
	case 1: 
		if(desmume_running()) {
			click = TRUE;	
			gdk_window_get_pointer(widget->window, &x, &y, &state);
			if (state & GDK_BUTTON1_MASK)
				set_touch_pos(x,y);
		}
		break;
	case 3: break;
	case 2: 
		ScreenCoeff_Size[0]=1.0;
		//ScreenCoeff_Size[1]=1.0; // separate zoom factors
		resize(ScreenCoeff_Size[0],ScreenCoeff_Size[1]); break;
	}
	return TRUE;
}

gboolean  on_wDraw_Sub_button_release_event (GtkWidget *widget, GdkEventButton  *event, gpointer user_data) {
	if(click) NDS_releasTouch();
	click = FALSE;
	return TRUE;
}

static void resize_incremental(int i, GdkEventScroll *event) {
#ifdef HAVE_LIBGDKGLEXT_X11_1_0
	float zoom_inc=.125, zoom_min=0.25, zoom_max=5.0;
#else
	float zoom_inc=1.0, zoom_min=1.0, zoom_max=3.0;
#endif
	switch (event->direction) {
	case GDK_SCROLL_UP:
		ScreenCoeff_Size[i]=MIN(ScreenCoeff_Size[i]+zoom_inc,zoom_max); break;
	case GDK_SCROLL_DOWN:
		ScreenCoeff_Size[i]=MAX(ScreenCoeff_Size[i]-zoom_inc,zoom_min); break;
	case GDK_SCROLL_LEFT:
	case GDK_SCROLL_RIGHT:
		return;
	}
	resize(ScreenCoeff_Size[0],ScreenCoeff_Size[1]);
}

gboolean  on_wDraw_Main_scroll_event (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	resize_incremental(0,event);
}
gboolean  on_wDraw_Sub_scroll_event  (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	// using separate zoom factors is bad for now
	resize_incremental(0,event);
//	resize_incremental(1,event);
}



gboolean  on_wDraw_Sub_motion_notify_event  (GtkWidget *widget, GdkEventMotion  *event, gpointer user_data) {
	GdkModifierType state;
	gint x,y;
	
	if(click)
	{
		if(event->is_hint)
			gdk_window_get_pointer(widget->window, &x, &y, &state);
		else
		{
			x= (gint)event->x;
			y= (gint)event->y;
			state=(GdkModifierType)event->state;
		}
		
	//	fprintf(stderr,"X=%d, Y=%d, S&1=%d\n", x,y,state&GDK_BUTTON1_MASK);
	
		if(state & GDK_BUTTON1_MASK)
			set_touch_pos(x,y);
	}
	
	return TRUE;
}




/* ***** ***** KEYBOARD CONFIG / KEY DEFINITION ***** ***** */
u16 Keypad_Temp[NB_KEYS];
guint temp_Key=0;

void init_labels() {
	int i;
	char text[50], bname[20];
	GtkButton *b;
	for (i=0; i<NB_KEYS; i++) {
		sprintf(text,"%s : %s\0\0",key_names[i],KEYNAME(keyboard_cfg[i]));
		sprintf(bname,"button_%s\0\0",key_names[i]);
		b = (GtkButton*)glade_xml_get_widget(xml, bname);
		gtk_button_set_label(b,text);
	}
}

/* Initialize the joystick controls labels for the configuration window */
void init_joy_labels() {
  int i;
  char text[50], bname[30];
  GtkButton *b;
  for (i=0; i<NB_KEYS; i++) {
    if( joypad_cfg[i] == (u16)(-1) ) continue; /* Key not configured */
    sprintf(text,"%s : %d\0\0",key_names[i],joypad_cfg[i]);
    sprintf(bname,"button_joy_%s\0\0",key_names[i]);
    b = (GtkButton*)glade_xml_get_widget(xml, bname);
    gtk_button_set_label(b,text);
  }
}

void edit_controls() {
	GtkDialog * dlg = (GtkDialog*)glade_xml_get_widget(xml, "wKeybConfDlg");
	memcpy(&Keypad_Temp, &keyboard_cfg, sizeof(keyboard_cfg));
	/* we change the labels so we know keyb def */
	init_labels();
	gtk_widget_show((GtkWidget*)dlg);
}

void  on_wKeybConfDlg_response (GtkDialog *dialog, gint arg1, gpointer user_data) {
	/* overwrite keyb def if user selected ok */
	if (arg1 == GTK_RESPONSE_OK)
		memcpy(&keyboard_cfg, &Keypad_Temp, sizeof(keyboard_cfg));
	gtk_widget_hide((GtkWidget*)dialog);
}

void inline current_key_label() {
	GtkLabel * lbl = (GtkLabel*)glade_xml_get_widget(xml, "label_key");
	gtk_label_set_text(lbl, KEYNAME(temp_Key));
}

gboolean  on_wKeyDlg_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	temp_Key = event->keyval;
	current_key_label();
	return TRUE;
}

void ask(GtkButton*b, int key) {
	char text[50];
	GtkDialog * dlg = (GtkDialog*)glade_xml_get_widget(xml, "wKeyDlg");
	key--; /* key = bit position, start with 1 */
	temp_Key = Keypad_Temp[key];
	current_key_label();
	switch (gtk_dialog_run(dlg))
	{
		case GTK_RESPONSE_OK:
			Keypad_Temp[key]=temp_Key;
			sprintf(text,"%s : %s\0\0",key_names[key],KEYNAME(temp_Key));
			gtk_button_set_label(b,text);
			break;
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
			break;
	}
	gtk_widget_hide((GtkWidget*)dlg);
}

void  on_button_Left_clicked    (GtkButton *b, gpointer user_data) { ask(b,KEY_LEFT); }
void  on_button_Up_clicked      (GtkButton *b, gpointer user_data) { ask(b,KEY_UP); }
void  on_button_Right_clicked   (GtkButton *b, gpointer user_data) { ask(b,KEY_RIGHT); }
void  on_button_Down_clicked    (GtkButton *b, gpointer user_data) { ask(b,KEY_DOWN); }

void  on_button_L_clicked       (GtkButton *b, gpointer user_data) { ask(b,KEY_L); }
void  on_button_R_clicked       (GtkButton *b, gpointer user_data) { ask(b,KEY_R); }

void  on_button_Y_clicked       (GtkButton *b, gpointer user_data) { ask(b,KEY_Y); }
void  on_button_X_clicked       (GtkButton *b, gpointer user_data) { ask(b,KEY_X); }
void  on_button_A_clicked       (GtkButton *b, gpointer user_data) { ask(b,KEY_A); }
void  on_button_B_clicked       (GtkButton *b, gpointer user_data) { ask(b,KEY_B); }

void  on_button_Start_clicked   (GtkButton *b, gpointer user_data) { ask(b,KEY_START); }
void  on_button_Select_clicked  (GtkButton *b, gpointer user_data) { ask(b,KEY_SELECT); }
void  on_button_Debug_clicked   (GtkButton *b, gpointer user_data) { ask(b,KEY_DEBUG); }
void  on_button_Boost_clicked   (GtkButton *b, gpointer user_data) { ask(b,KEY_BOOST); }

/* Joystick configuration / Key definition */
void ask_joy_key(GtkButton*b, int key)
{
  char text[50];
  u16 joykey;

  GtkWidget * dlg = (GtkWidget*)glade_xml_get_widget(xml, "wJoyDlg");

  key--; /* remove 1 to get index */
  gtk_widget_show_now(dlg);
  /* Need to force event processing. Otherwise, popup won't show up. */
  while ( gtk_events_pending() ) gtk_main_iteration();
  joykey = get_set_joy_key(key);
  sprintf(text,"%s : %d\0\0",key_names[key],joykey);
  gtk_button_set_label(b,text);
  gtk_widget_hide((GtkWidget*)dlg);
}

/* Joystick configuration / Key definition */
void ask_joy_axis(GtkButton*b, u8 key, u8 opposite_key)
{
  char text[50];
  char opposite_button[50];
  u16 joykey;
  GtkWidget * dlg;
  GtkWidget * bo;

  key--; /* remove 1 to get index */
  opposite_key--;

  sprintf(opposite_button,"button_joy_%s\0\0",key_names[opposite_key]);
  dlg = (GtkWidget*)glade_xml_get_widget(xml, "wJoyDlg");
  bo = (GtkWidget*)glade_xml_get_widget(xml, opposite_button);

  gtk_widget_show(dlg);
  /* Need to force event processing. Otherwise, popup won't show up. */
  while ( gtk_events_pending() ) gtk_main_iteration();
  get_set_joy_axis(key, opposite_key);

  sprintf(text,"%s : %d\0\0",key_names[key],joypad_cfg[key]);
  gtk_button_set_label(b,text);

  sprintf(text,"%s : %d\0\0",key_names[opposite_key],joypad_cfg[opposite_key]);
  gtk_button_set_label(bo,text);

  gtk_widget_hide((GtkWidget*)dlg);
}

void on_button_joy_Left_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_axis(b,KEY_LEFT,KEY_RIGHT); }
void on_button_joy_Up_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_axis(b,KEY_UP,KEY_DOWN); }
void on_button_joy_Right_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_axis(b,KEY_RIGHT,KEY_LEFT); }
void on_button_joy_Down_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_axis(b,KEY_DOWN,KEY_UP); }
void on_button_joy_A_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_A); }
void on_button_joy_B_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_B); }
void on_button_joy_X_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_X); }
void on_button_joy_Y_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_Y); }
void on_button_joy_L_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_L); }
void on_button_joy_R_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_R); }
void on_button_joy_Select_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_SELECT); }
void on_button_joy_Start_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_START); }
void on_button_joy_Boost_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_BOOST); }
void on_button_joy_Debug_clicked (GtkButton *b, gpointer user_data)
{ ask_joy_key(b,KEY_DEBUG); }
