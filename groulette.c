#include<gtk/gtk.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>


#define XOFFSET 252
#define YOFFSET 18
#define CELL_WIDTH 32
#define CELL_HEIGHT 37
#define ZEROCELL_WIDTH CELL_WIDTH
#define ZEROCELL_HEIGHT ((CELL_HEIGHT*3)/2)
#define LONGCELL_WIDTH (CELL_WIDTH*4)
#define LONGCELL_HEIGHT 23
#define RED 1
#define BLACK 2

#define debug(...) printf(__VA_ARGS__)

static int COLOR[38] = {0, RED, BLACK, RED, BLACK, RED, BLACK, RED, BLACK, RED, BLACK, BLACK, RED,
                        BLACK, RED, BLACK, RED, BLACK, RED, BLACK, BLACK, RED, BLACK, RED, BLACK,
                        RED, BLACK, RED, RED, BLACK, RED, BLACK, RED, BLACK, RED, BLACK, RED, 0};

struct roulpos {
	char name[80];
	uint32_t bet;
};

struct groulette {
	uint32_t funds;
	uint32_t round;
	uint32_t bets;
	struct roulpos num_bets[38];
	struct roulpos col_bets[3];
	struct roulpos third_bets[3];
	struct roulpos halves_bets[2];
	struct roulpos odd_bets;
	struct roulpos even_bets;
	struct roulpos red_bets;
	struct roulpos black_bets;

	struct roulpos *pot;
	GtkBuilder *builder;
};

void groulette_reset(struct groulette *r)
{
	int i;

	r->bets = 0;
	for (i=0; i<37; i++) {
		sprintf(r->num_bets[i].name, "%d", i);
		r->num_bets[i].bet = 0;
	}
	sprintf(r->num_bets[37].name, "00");
	r->num_bets[37].bet = 0;
	for (i=0; i<3; i++) {
		sprintf(r->col_bets[i].name, "%d^ column", i+1);
		r->col_bets[i].bet = 0;
	}
	for (i=0; i<3; i++) {
		sprintf(r->third_bets[i].name, "%d-%d", i*12+1, (i+1)*12);
		r->third_bets[i].bet = 0;
	}
	for (i=0; i<2; i++) {
		sprintf(r->halves_bets[i].name, "%d-%d", i*18+1, (i+1)*18);
		r->halves_bets[i].bet = 0;
	}
	sprintf(r->odd_bets.name, "odd");
	r->odd_bets.bet = 0;
	sprintf(r->even_bets.name, "even");
	r->even_bets.bet = 0;
	sprintf(r->red_bets.name, "red");
	r->red_bets.bet = 0;
	sprintf(r->black_bets.name, "black");
	r->black_bets.bet = 0;
}

void groulette_init(struct groulette *r)
{
	r->funds = 100;
	r->round = 1;
	r->builder = NULL;

	groulette_reset(r);
	r->pot=&(r->num_bets[37]);
}

struct roulpos* coords_to_pot(struct groulette *r, int x, int y)
{
	int n;
	struct roulpos * res = NULL;

	x -= XOFFSET;
	y -= YOFFSET;
	if (y>=0) {
		if (x>=0 && x< CELL_WIDTH*12 && y< CELL_HEIGHT*3) {
			// main number board
			x /= CELL_WIDTH;
			y /= CELL_HEIGHT;
			y = 2 - y;  // upside-down
			n = y + x*3 + 1;
			debug("Guess: %d\n", n);
			res = &(r->num_bets[n]);
		}
		if (!res && x<0 && x >= -ZEROCELL_WIDTH && y < ZEROCELL_HEIGHT*2) {
			// zeros number board
			y /= ZEROCELL_HEIGHT;
			n = (y == 0 ? 37 : 0);
			debug("Guess: %s\n", n ? "00" : "0");
			res = &(r->num_bets[n]);
		}
		if (!res && x>=0 && x < CELL_WIDTH*12 && y < CELL_HEIGHT*3 + LONGCELL_HEIGHT) {
			x /= LONGCELL_WIDTH;
			res = &(r->third_bets[x]);
		}
		if (!res && x>=0 && x < CELL_WIDTH*12 && y < CELL_HEIGHT*3 + LONGCELL_HEIGHT*2) {
			x /= (LONGCELL_WIDTH/2);
			switch (x) {
				case 0:
					res = &(r->halves_bets[0]);
					break;
				case 1:
					res = &(r->even_bets);
					break;
				case 2:
					res = &(r->red_bets);
					break;
				case 3:
					res = &(r->black_bets);
					break;
				case 4:
					res = &(r->odd_bets);
					break;
				default:
					res = &(r->halves_bets[1]);
					break;
			}
		}
		if (!res && x>= CELL_WIDTH*12 && x< CELL_WIDTH*13 && y < CELL_HEIGHT*3) {
			y /= CELL_HEIGHT;
			y = 2 - y;
			res = &(r->col_bets[y]);
		}
	}
	return res;
}

void on_adjustment1_changed(GtkAdjustment *adj, struct groulette *r)
{

}

void groulette_refresh_funds_panel(struct groulette *r)
{
	char text[100];
	GtkLabel * lbl;

	lbl = GTK_LABEL(gtk_builder_get_object(r->builder, "funds"));
	snprintf(text, 100, "Funds: %d§", r->funds);
	gtk_label_set_text(lbl, text);
	lbl = GTK_LABEL(gtk_builder_get_object(r->builder, "round"));
	snprintf(text, 100, "Round: %d", r->round);
	gtk_label_set_text(lbl, text);
	lbl = GTK_LABEL(gtk_builder_get_object(r->builder, "bets"));
	snprintf(text, 100, "Bets: %d§", r->bets);
	gtk_label_set_text(lbl, text);
}

void groulette_refresh_bet_panel(struct groulette *r)
{
	char text[100];
	GtkLabel * lbl;
	GtkAdjustment *adj;

	if (r->pot) {
		lbl = GTK_LABEL(gtk_builder_get_object(r->builder, "selection"));
		snprintf(text, 100, "Selected: %s", r->pot->name);
		gtk_label_set_text(lbl, text);

		adj = GTK_ADJUSTMENT(gtk_builder_get_object(r->builder, "adjustment1"));
		g_object_freeze_notify(G_OBJECT(adj));
		gtk_adjustment_set_value(adj, r->pot->bet);
		gtk_adjustment_set_upper(adj, r->funds + r->pot->bet);
		g_object_thaw_notify(G_OBJECT(adj));
	}
}

gboolean btn_pressed_cb(GtkWidget *widget, GdkEvent *ev, gpointer cb_data)
{
	GdkEventButton * event;
	struct groulette * data;
	struct roulpos * pot;
	data = (struct groulette*) cb_data;

	event = (GdkEventButton*) ev;
	if (event->button == 1) { // left click
		debug("Click! (%f, %f)\n", event->x, event->y);
		pot = coords_to_pot(data, (int) event->x, (int) event->y);
		if (pot) {
			data->pot = pot;
			groulette_refresh_bet_panel(data);
		}
	}

	return TRUE;
}

void on_place_clicked(GtkButton *btn, struct groulette *gr)
{
	uint32_t bet;
	GtkAdjustment * adj;

	adj = GTK_ADJUSTMENT(gtk_builder_get_object(gr->builder, "adjustment1"));
	bet = gtk_adjustment_get_value(adj);

	gr->funds -= bet - gr->pot->bet;
	gr->bets += bet - gr->pot->bet;
	gr->pot->bet = bet;

	groulette_refresh_funds_panel(gr);
}

void groulette_advertise_win(struct groulette *gr, int res, int gain)
{
	GtkWidget * dialog;
	GtkWindow * win;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	char msg[80];

	if (res < 37)
		snprintf(msg, 80, "Number %d!\n Your total gain is %d", res, gain);
	else
		snprintf(msg, 80, "Number 00!\n Your total gain is %d", gain);

	win = GTK_WINDOW(gtk_builder_get_object(gr->builder, "main_win"));
	dialog = gtk_message_dialog_new (win,
	                                 flags,
	                                 GTK_MESSAGE_INFO,
	                                 GTK_BUTTONS_OK,
	                                 msg);

	// Destroy the dialog when the user responds to it
	// (e.g. clicks a button)

	g_signal_connect_swapped (dialog, "response",
	                          G_CALLBACK (gtk_widget_destroy),
	                          dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));
}

int groulette_handle_result(struct groulette *gr, int res)
{
	int gain = 0;
	int col, third, half;

	if (gr->num_bets[res].bet)
		gain += gr->num_bets[res].bet * 36;

	if (res && res < 37) {
		col = (res-1) % 3;
		if (gr->col_bets[col].bet)
			gain += gr->col_bets[col].bet * 3;

		third = (res-1)/12;
		if (gr->third_bets[third].bet)
			gain += gr->third_bets[third].bet * 3;

		half = (res <19 ? 0 : 1);
		if (gr->halves_bets[half].bet)
			gain += gr->halves_bets[half].bet * 2;

		if (res % 2 && gr->odd_bets.bet)
			gain += gr->odd_bets.bet * 2;
		if ((res % 2 == 0) && gr->even_bets.bet)
			gain += gr->even_bets.bet * 2;

		if (COLOR[res] == RED && gr->red_bets.bet)
			gain += gr->red_bets.bet * 2;
		if (COLOR[res] == BLACK && gr->black_bets.bet)
			gain += gr->black_bets.bet * 2;
	}
	groulette_advertise_win(gr, res, gain);

	return gain;
}

void groulette_animate_result(struct groulette *gr, int num)
{
	GtkLabel * lbl;
	char text[4];

	lbl = GTK_LABEL(gtk_builder_get_object(gr->builder, "number"));

	if (num < 37)
		snprintf(text, 4, "%d", num);
	else
		snprintf(text, 4, "00");
	gtk_label_set_text(lbl, text);
}

void on_spin_clicked(GtkButton *btn, struct groulette *gr)
{
	int num, gain;

	num = rand()%38;  // "00" is 37
	groulette_animate_result(gr, num);
	gain = groulette_handle_result(gr, num);
	groulette_reset(gr);
	gr->round++;
	gr->funds += gain;
	groulette_refresh_funds_panel(gr);
	groulette_refresh_bet_panel(gr);

	debug("Number out: %d\tGain: %d\n", num, gain);
}

int main(int argc, char **argv)
{
	GtkWidget * win;
	struct groulette data;
	groulette_init(&data);

	srand(time(NULL));
	gtk_init(&argc, &argv);

	data.builder = gtk_builder_new();
	gtk_builder_add_from_file(data.builder, "groulette.glade", NULL);
	gtk_builder_connect_signals(data.builder, &data);
	

	win = GTK_WIDGET(gtk_builder_get_object(data.builder, "main_win"));
	g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(win, "button-press-event", G_CALLBACK(btn_pressed_cb), &data);

	groulette_refresh_funds_panel(&data);
	groulette_refresh_bet_panel(&data);

	gtk_widget_show(win);
	gtk_main();

	g_object_unref(data.builder);
	return 0;
}
