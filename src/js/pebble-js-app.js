var SERVER_URL = 'http://covrme-dev-armstrong-timothy.appspot.com';
var AUTH_TOKEN = 'e7ljbthr9a693opo7od3cqqk7c4m7sse';
var DOORBELL_ID = '65432353';


var currentVisitorId = '';

/**
 * Retrieves the current status of whether there is someone at the door.
 * The status is sent back to the watch after it is retrieved.
 */
function getStatus() {
  var xhr = new XMLHttpRequest();
  var url = SERVER_URL + '/doorbells/' + DOORBELL_ID + '/visitors?authtoken=' + AUTH_TOKEN;
  xhr.open('GET', url, true);

  xhr.onload = function(e) {
    var data = JSON.parse(this.response);
    if (data.length > 0) {
      var mostRecentVisitor = data[0];
      var currentTime = (new Date()).getTime();
      var visitorTime = (new Date(Date.parse(mostRecentVisitor.when))).getTime();
      var timeDelta = currentTime - visitorTime;
      if (timeDelta > 0 && timeDelta < 60000) {
        Pebble.sendAppMessage({
          status: 1,
          visitor_description: mostRecentVisitor.description
        });
        currentVisitorId = mostRecentVisitor.id;
        return;
      }
    }
    // No one is at the door.
    Pebble.sendAppMessage({
      status: 0
    });
    currentVisitorId = '';
  };
  xhr.send();
}


/**
 * Sends a message to the visitor at the door.
 * @param {string} message The message to send.
 */
function sendResponseMessage(message) {
  var xhr = new XMLHttpRequest();
  var url = SERVER_URL + '/doorbells/' + DOORBELL_ID + '/visitors/' + currentVisitorId + '/messages';
  xhr.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
  xhr.open('POST', url, true);
  xhr.send('authtoken=' + AUTH_TOKEN + '&message=' + encodeURIComponent(message));
}

Pebble.addEventListener('ready', function(e) {
  console.log('JS: ready.');
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('JS: appmessage');
  if (e.payload.fetch) {
    getStatus();
  } else if (e.payload.response_message) {
    sendResponseMessage(e.payload.response_message);
  }
});
