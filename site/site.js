// The demo recording moves. It loads at once, unless the visitor asked the system
// for reduced motion: then it waits for a click.
(function () {
  var frame = document.getElementById('demo-frame');
  if (!frame) return;

  function play() {
    var img = document.createElement('img');
    img.src = frame.getAttribute('data-src');
    img.alt = frame.getAttribute('data-alt');
    img.width = 320;
    img.height = 480;
    frame.textContent = '';
    frame.appendChild(img);
  }

  var calm = window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  if (!calm) {
    play();
    return;
  }
  var still = document.createElement('div');
  still.className = 'demo-still';
  var button = document.createElement('button');
  button.type = 'button';
  button.textContent = 'Play the recording';
  button.addEventListener('click', play);
  still.appendChild(button);
  frame.appendChild(still);
})();
