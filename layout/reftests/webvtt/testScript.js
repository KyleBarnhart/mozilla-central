/*
   Make sure video is loaded,
   and that it is always at the same frame.
*/
function loadVideo() {
	var testVideo = document.getElementById('testVideo');

	// Need to play to load video
	testVideo.onplay = function() {
		
		// Stop video and seek to 3 seconds
		testVideo.onpause = function() {
		
			// When video is loaded, preform test
			testVideo.oncanplaythrough = function() {
				document.documentElement.className = "";
			};
			testVideo.currentTime = 3;
		};
		testVideo.pause();
	};
	testVideo.play();
}