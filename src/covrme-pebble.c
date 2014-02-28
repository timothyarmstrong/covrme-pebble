#include <pebble.h>

enum {
  COVRME_KEY_STATUS = 0x0,
  COVRME_KEY_VISITOR_DESCRIPTION = 0x1,
  COVRME_KEY_FETCH = 0x2,
  COVRME_KEY_RESPONSE_MESSAGE = 0x3,
};

typedef struct {
  char* line1;
  char* line2;
} ResponseMessage;

ResponseMessage response_messages[] = {
  { .line1 = "I'm on my way!", .line2 = "" },
  { .line1 = "I'm not available", .line2 = "...right now." },
  { .line1 = "Go away!", .line2 = "" },
};

#define NUM_RESPONSE_MESSAGES sizeof(response_messages) / sizeof(ResponseMessage);

static Window* main_window;
static TextLayer* title_layer;
static TextLayer* status_layer;

static Window* message_window;
static MenuLayer* menu_layer;


// Current status.
static int loading = 0;
static int door_status = 0;
static char visitor_description[32];


/**
 * Updates the current status displayed on the main screen.
 */
static void update_status() {
  if (loading) {
    text_layer_set_text(status_layer, "Loading...");
  } else if (door_status) {
    text_layer_set_text(status_layer, visitor_description);
  } else {
    text_layer_set_text(status_layer, "No one is at the door right now.");
  }
}


/**
 * Sends a message to the JS app, asking for the current status.
 */
static void get_door_status() {
  Tuplet fetch_tuplet = TupletInteger(COVRME_KEY_FETCH, 1);

  DictionaryIterator* iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    // TODO: Logging.
    return;
  }

  dict_write_tuplet(iter, &fetch_tuplet);
  dict_write_end(iter);

  app_message_outbox_send();

  loading = 1;
  update_status();
}


static void handle_message_received(DictionaryIterator* iter, void* context) {
  Tuple* status = dict_find(iter, COVRME_KEY_STATUS);
  Tuple* description = dict_find(iter, COVRME_KEY_VISITOR_DESCRIPTION);

  if (status) {
    if (!door_status && status->value->int32) {
      static const uint32_t const segments[] = {
        100, 100,
        100, 100,
        100, 100
      };
      VibePattern pattern = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments),
      };
      vibes_enqueue_custom_pattern(pattern);
    }
    door_status = status->value->int32;
  }
  if (description) {
    strncpy(visitor_description, description->value->cstring, 32);
  }

  loading = 0;
  update_status();
}


// Called when a message in the inbox is dropped.
static void handle_message_dropped(AppMessageResult reason, void* context) {
  // TODO: Log this, and maybe show to user.
}


static void handle_message_failed(DictionaryIterator* iter,
                                  AppMessageResult reason, void* context) {
  // TODO: Log this, and maybe show to user.
}


static void app_message_init() {
  app_message_register_inbox_received(handle_message_received);
  app_message_register_inbox_dropped(handle_message_dropped);
  app_message_register_outbox_failed(handle_message_failed);

  app_message_open(64, 64);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_init");
}


static void refresh_click_handler(ClickRecognizerRef recognizer,
                                  void* context) {
  get_door_status();
}

static void send_message_click_handler(ClickRecognizerRef recognizer,
                                       void* context) {
  if (door_status) {
    const bool animated = true;
    window_stack_push(message_window, animated);
  }
}

static void main_window_click_config_provider(void *context) {
  window_single_click_subscribe(
      BUTTON_ID_SELECT, (ClickHandler) refresh_click_handler);
  window_single_click_subscribe(
      BUTTON_ID_DOWN, (ClickHandler) send_message_click_handler);
}

static void main_window_load(Window *window) {
  // Listen for clicks.
  window_set_click_config_provider(window, main_window_click_config_provider);

  // Get information about the current window.
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the text layer to put the title in.
  title_layer = text_layer_create((GRect) {
    .origin = { 0, 20 },
    .size = { bounds.size.w, 30 }
  });
  text_layer_set_text(title_layer, "COVR.ME");
  text_layer_set_font(title_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(title_layer));

  // Create the text layer to put the current door status in.
  status_layer = text_layer_create((GRect) {
    .origin = { 5, bounds.size.h / 2 },
    .size = { bounds.size.w - 10, 28 * 2 }
  });
  text_layer_set_font(status_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(status_layer));

  // Fetch the initial status. This will put the initial loading into the status
  // text layer.
  get_door_status();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(title_layer);
  text_layer_destroy(status_layer);
}



static uint16_t get_num_menu_rows(struct MenuLayer *menu_layer,
                              uint16_t section_index, void *callback_context) {
  return NUM_RESPONSE_MESSAGES;
}



static void draw_menu_row(GContext* ctx, const Layer* cell_layer,
                          MenuIndex* cell_index, void* callback_context) {
  ResponseMessage* messages = (ResponseMessage*) callback_context;
  ResponseMessage* message = &messages[cell_index->row];

  menu_cell_basic_draw(ctx, cell_layer, message->line1, message->line2, NULL);
}


static void menu_item_click(struct MenuLayer *menu_layer, MenuIndex *cell_index,
                            void *callback_context) {
  // TODO: Send the message.
  ResponseMessage* messages = (ResponseMessage*) callback_context;
  ResponseMessage* message = &messages[cell_index->row];

  // TODO: Not just line1.
  Tuplet response_tuplet = TupletCString(COVRME_KEY_RESPONSE_MESSAGE, message->line1);

  DictionaryIterator* iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    // TODO: Logging.
    return;
  }

  dict_write_tuplet(iter, &response_tuplet);
  dict_write_end(iter);

  app_message_outbox_send();

  const bool animated = true;
  window_stack_pop(animated);
}


MenuLayerCallbacks menu_callbacks = {
  .get_num_rows = get_num_menu_rows,
  .draw_row = draw_menu_row,
  .select_click = menu_item_click,
};

static void message_window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(menu_layer, response_messages, menu_callbacks);
  menu_layer_set_click_config_onto_window(menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void message_window_unload(Window* window) {
  menu_layer_destroy(menu_layer);
}



static void init(void) {
  app_message_init();

  // Create and setup the main window.
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });


  // Create and setup the messages window.
  message_window = window_create();
  window_set_window_handlers(message_window, (WindowHandlers) {
    .load = message_window_load,
    .unload = message_window_unload,
  });

  // Display the main window.
  const bool animated = true;
  window_stack_push(main_window, animated);
}

static void deinit(void) {
  window_destroy(main_window);
  window_destroy(message_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", main_window);

  app_event_loop();
  deinit();
}
