function doGet() {
  return ContentService.createTextOutput(listUpcomingEvents()).setMimeType(ContentService.MimeType.JSON);
}

function listUpcomingEvents() {
  var calendarId = 'primary'; // Change this to a specific calendar ID if needed
  var now = new Date();
  var events = CalendarApp.getCalendarById(calendarId).getEvents(now, new Date(now.getTime() + (21 * 24 * 60 * 60 * 1000))); // Next 7 days
  
  var eventList = events.map(function(event) {
    return {
      title: event.getTitle(),
      startTime: event.getStartTime(),
      endTime: event.getEndTime(),
      description: event.getDescription(),
      location: event.getLocation(),
      creators: event.getCreators()
    };
  });
  
  return JSON.stringify(eventList, null, 2);
}
