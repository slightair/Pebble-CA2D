#include <pebble.h>

static int s_seed;
static Window *s_main_window;
static TextLayer *s_time_layer;
static Layer *s_ca_layer;
static char *s_board;
static int s_step;

#define NUM_CELLS 18
#define CELL_SIZE 8

// Star Wars 345/2/4
#define SURVIVE ((1 << 2) + (1 << 3) + (1 << 4))
#define BORN 2
#define CONDITIONS 4
#define STEP_MAX 100
  
static int random() {
	s_seed = (((s_seed * 214013L + 2531011L) >> 16) & 32767);
	return s_seed;
}

static void update_time() {
  time_t current_time = time(NULL);
  struct tm *tick_time = localtime(&current_time);
  static char buffer[] = "00:00";
  int len = sizeof("00:00");
  
  if(clock_is_24h_style()) {
    strftime(buffer, len, "%H:%M", tick_time);
  } else {
    strftime(buffer, len, "%I:%M", tick_time);
  }
  text_layer_set_text(s_time_layer, buffer);
}

static void shuffle_board() {
  for (int x = 0; x<NUM_CELLS; x++) {
    for (int y = 0; y<NUM_CELLS; y++) {
      int index = y * NUM_CELLS + x;
      
      if (random() % 16 == 0) {
        s_board[index] = CONDITIONS - 1;
      } else {
        s_board[index] = 0;
      }      
    }
  }    
}

static void tick_board() {  
  char *prevBoard = s_board;
  char *newBoard = malloc(NUM_CELLS * NUM_CELLS * sizeof(char));
  
  int x, y, i, j, count, surviveCount;
  int w = NUM_CELLS;
  int h = NUM_CELLS;
  int lastX = w - 1;
  int lastY = h - 1;
  char condMax = CONDITIONS - 1;
  
  surviveCount = 0;
  for(y=0;y<h;y++){
    for(x=0;x<w;x++){
      count = 0;
      
      if(y==0){
        // top edge
        if(x==0){
          // left edge
          if(prevBoard[lastX + lastY * w] == condMax)count++;
          if(prevBoard[0 + lastY * w] == condMax)count++;
          if(prevBoard[1 + lastY * w] == condMax)count++;
          if(prevBoard[lastX + 0 * w] == condMax)count++;
          if(prevBoard[lastX + 1 * w] == condMax)count++;
          
          for(j=0;j<=1;j++){
            for(i=0;i<=1;i++){
              if(prevBoard[i + j * w] == condMax)count++;
            }
          }
        }
        else if(x==lastX){
          // right edge
          if(prevBoard[(lastX-1) + lastY * w] == condMax)count++;
          if(prevBoard[lastX + lastY * w] == condMax)count++;
          if(prevBoard[0 + lastY * w] == condMax)count++;
          if(prevBoard[0 + 0 * w] == condMax)count++;
          if(prevBoard[0 + 1 * w] == condMax)count++;
          
          for(j=0;j<=1;j++){
            for(i=lastX-1;i<=lastX;i++){
              if(prevBoard[i + j * w] == condMax)count++;
            }
          }
        }
        else{
          for(i=x-1;i<=x+1;i++){
            if(prevBoard[i + lastY * w] == condMax)count++;
          }
          
          for(j=0;j<=1;j++){
            for(i=x-1;i<=x+1;i++){
              if(prevBoard[i + j * w] == condMax)count++;
            }
          }
        }
      }
      else if(y==lastY){
        // bottom edge
        if(x==0){
          // left edge
          for(j=lastY-1;j<=lastY;j++){
            for(i=0;i<=1;i++){
              if(prevBoard[i + j * w] == condMax)count++;
            }
          }
          if(prevBoard[lastX + (lastY-1) * w] == condMax)count++;
          if(prevBoard[lastX + lastY * w] == condMax)count++;
          if(prevBoard[lastX + 0 * w] == condMax)count++;
          if(prevBoard[0 + 0 * w] == condMax)count++;
          if(prevBoard[1 + 0 * w] == condMax)count++;
        }
        else if(x==lastX){
          // right edge
          for(j=lastY-1;j<=lastY;j++){
            for(i=lastX-1;i<=lastX;i++){
              count += prevBoard[i + j * w] == condMax ? 1 : 0;
            }
          }
          count += prevBoard[0 + (lastY-1) * w] == condMax ? 1 : 0;
          count += prevBoard[0 + lastY * w] == condMax ? 1 : 0;
          count += prevBoard[(lastX-1) + 0 * w] == condMax ? 1 : 0;
          count += prevBoard[lastX + 0 * w] == condMax ? 1 : 0;
          count += prevBoard[0 + 0 * w] == condMax ? 1 : 0;
        }
        else{
          for(i=x-1;i<=x+1;i++){
            if(prevBoard[i + 0 * w] == condMax)count++;
          }
          
          for(j=lastY-1;j<=lastY;j++){
            for(i=x-1;i<=x+1;i++){
              if(prevBoard[i + j * w] == condMax)count++;
            }
          }
        }
      }
      else if(x==0){
        // left edge
        for(j=y-1;j<=y+1;j++){
          for(i=0;i<=1;i++){
            if(prevBoard[i + j * w] == condMax)count++;
          }
          if(prevBoard[lastX + j * w] == condMax)count++;
        }
      }
      else if(x==lastX){
        // right edge
        for(j=y-1;j<=y+1;j++){
          for(i=lastX-1;i<=lastX;i++){
            count += prevBoard[i + j * w] == condMax ? 1 : 0;
          }
          count += prevBoard[0 + j * w] == condMax ? 1 : 0;
        }
      }
      else{
        for(j=y-1;j<=y+1;j++){
          for(i=x-1;i<=x+1;i++){
            if(prevBoard[i + j * w] == condMax)count++;
          }
        }
      }
      
      if(prevBoard[x + y * w] == condMax)count--;
      
      int env = 0;
      if(count > 0){
        env = (1 << (count - 1));
      } 
      
      char prevCond = prevBoard[x + y * w];
      if(prevCond == 0 && BORN & env){
        newBoard[x + y * w] = condMax;
      }
      else if(prevCond == condMax && SURVIVE & env){
        newBoard[x + y * w] = prevCond;
      }
      else{
        if(prevCond > 0){
          newBoard[x + y * w] = prevCond - 1;
        }
        else{
          newBoard[x + y * w] = 0;
        }
      }
      
      surviveCount += newBoard[x + y * w];
    }
  }
  
  s_board = newBoard;
  free(prevBoard);
  
  s_step++;
  if (s_step > STEP_MAX || surviveCount == 0) {
    shuffle_board();
    s_step = 0;
  }
}

static void ca_update_callback(Layer *me, GContext *context) {
  graphics_context_set_fill_color(context, GColorWhite);
  
  tick_board();
  
  for (int x = 0; x<NUM_CELLS; x++) {
    for (int y = 0; y<NUM_CELLS; y++) {
      char status = s_board[y * NUM_CELLS + x];
      if (status > 0) {
        graphics_fill_circle(context, GPoint(CELL_SIZE / 2 + CELL_SIZE * x, CELL_SIZE / 2 + CELL_SIZE * y), status);   
      }
    }
  }  
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  window_set_background_color(window, GColorBlack);
  
  s_ca_layer = layer_create(bounds);
  layer_set_update_proc(s_ca_layer, ca_update_callback);
  layer_add_child(window_layer, s_ca_layer);
  
  s_time_layer = text_layer_create(GRect(0, 144, 144, 24));

  text_layer_set_background_color(s_time_layer, GColorWhite);
  text_layer_set_text_color(s_time_layer, GColorBlack);

  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));    
  
  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init() {
  s_seed = time(NULL);
  s_board = malloc(NUM_CELLS * NUM_CELLS * sizeof(char));
  s_step = 0;

  shuffle_board();

  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
  free(s_board);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}