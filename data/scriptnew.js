// Module for handling blinds-related functionality
const BlindsModule = (function () {
     // Private variables
    let blindName = "";
    let blindOpenFlag = 0;
    let limitSetupFlag = 0;
    let initializationFlag = 0;
    const data = {
      status : 0
    }

    const socket = new WebSocket('ws://' + window.location.hostname + '/ws');
    // Event handler when the WebSocket connection is opened
    // Promise that resolves when the WebSocket is opened
    const socketOpenPromise = new Promise((resolve) => {
      socket.addEventListener('open', (event) => {
        console.log('WebSocket connection opened');
        resolve();
      });
    });

    // Promise that resolves when the first message is received
    const firstMsgPromise = new Promise((resolve) => {
      socket.addEventListener('message', (event) => {
        const message = event.data;
            // Assuming event.data is always a string
        if (typeof message === 'string' && message.startsWith('LOG :')) {
          console.log('Log event received:', message);
        }else {
          try {
            const data = JSON.parse(message);
            console.log("-> ", JSON.stringify(data));
            handleReceiveBlindsInfo(data);
            resolve();
          } catch (e) {
            console.error('Error parsing message as JSON:', e);
          }
        }
      });
    });

    // very first request when loaded
    function wsInitialize(){
      const data ={
        initialize:1
      }
      const requestData = JSON.stringify(data);
      if (socket.readyState == WebSocket.OPEN){
        socket.send(requestData);
        console.log(" wsInitialize :", String(requestData));
      }
    }

    async function firstLoad(){
      await socketOpenPromise;
      console.log(" socket opening is done;")
      if (blindName == ""){
        wsInitialize();
        await firstMsgPromise;
        // Set controls container name    
        setControlsContainerName();
      }
    }

    // Event handler when the WebSocket connection is closed
    socket.addEventListener('close', (event) => {
      console.log('WebSocket connection closed');
    });

    // Event listener for errors in the WebSocket connection
    socket.addEventListener('error', (event) => {
      console.error('WebSocket error:', event);
    });

    function handleReceiveBlindsInfo(data){
      
      if (data.status !== undefined) {
        // Handle blinds status update
        var currentstatus = toggleButton.classList.contains("off");
        // blinds open & status here is close or vise-versa
        // changes the status of the button on screen only
        // if we received the changed signal;
        if (data.status != blindOpenFlag ){
          toggleButton.classList.toggle("off");
          blindOpenFlag=data.status;
          toggleButton.textContent = toggleButton.classList.contains("off") ? "Close" : "Open"; 
        }
      } 
      if (data.limitSetupFlag !== undefined) {
        // Handle limit setup flag update
              // Your logic to handle limit setup flag update
        try{
          limitSetupFlag = data.limitSetupFlag;
        } catch (error) {
          // Handle JSON parsing error
          alert("Error parsing server response: " + error.message);
        } finally {
          if (limitSetupFlag != 3) {
            // Remove 'waiting' class to change background color back
            momentaryButton.classList.remove("waiting");
            // Enable the button after receiving the server response
            // momentaryButton.disabled = false;
          } else {
            momentaryButton.textContent = "set";
            momentaryButton.disabled = true;
          }
        }
      }

      if (data.sliderPosition !== undefined){
        const slider = document.getElementById("openRangeSlider");
        slider.value = data.sliderPosition;
      }

      if (data.blindName !== undefined){
        var containerName = document.getElementById("controlTitle").textContent;
        if (blindName=="" || containerName == "Blinds-name" ){
          document.getElementById("controlTitle").textContent = containerName;
          document.title = containerName+ " Controls";
          toggleButton.textContent = 'Open';
        }
        blindName = data.blindName;
      }
    }

    // Function to set the name of the controls container
    function setControlsContainerName() { //GOOD
        var containerName ;
        var whichSideOfWindow;
        // request a first update from the site if it has already has the blindsname;
        if( blindName == "" ){
          containerName = prompt(" Important Notice: Make sure that the blinds are in completely\
            closed position as it would set as zero position \n \
                    Enter the name for the controls container:");
          // if control is on the left side then the switch will run other direction;
          
          if (containerName) {
            // Update the page title and controls container name
            document.getElementById("controlTitle").textContent = containerName;
            document.title = containerName + " Controls";
            
            toggleButton.textContent = 'Open';
            blindOpenFlag=0;
    
            whichSideOfWindow =prompt(" Is Control placed on right side YES/NO ? ", "YES");
            let requestData="blindName=" + containerName + "&rightSide="+ whichSideOfWindow.toUpperCase();
            fetch("/firstLoad", {
              method:'POST',
              headers: { 
                'Content-Type': 'application/x-www-form-urlencoded',
              },
              body: requestData,
            })
            .then(response => {
              if (!response.ok) {
                throw new Error('Network response was not ok');
              }
              return response.json();
            })
            .then(responsedata=> {
              // Successful response from the server
              handleReceiveBlindsInfo(responsedata);
              }
            )
            .catch( error => {console.error("setControlsContainer", error.message);})
          }
        }  else {
          document.title = blindName + " Controls";
          document.getElementById("controlTitle").textContent = blindName;
        }
    }

    // Function to get blind name (added for demonstration)
    function getBlindName() {
        return blindName;
    }

    // Function to send slider values over WebSocket
    function wsUpdateOpenRange(value) {
      // Create a JSON object with the slider value
      if (limitSetupFlag == 3){
        const data = {
          sliderValue: value
        };
        // Convert the JSON object to a string
        const requestData = JSON.stringify(data);

        // Check if the WebSocket connection is open before sending
        if (socket.readyState === WebSocket.OPEN) {
          // Send the slider value to the server via WebSocket
          socket.send(requestData);
        } else {
          console.error('WebSocket connection is not open.');
        }
      } else {
        alert("Windows limits have to be set first (both) !" + BlindsModule.limitSetupFlag);
        const openRangeSlider = document.getElementById("openRangeSlider");
        if (blindOpenFlag ==1){
          openRangeSlider.value =0 
        } else{
          openRangeSlider.value = 100
        }
      }
    }

    // Assuming you have opened a WebSocket connection named 'socket'
    function wsSendMomentaryToggle() {
      const data = {
        setupLimit: blindOpenFlag
      };
      // Add 'waiting' class to change background color
      if (limitSetupFlag!= 3){
        momentaryButton.classList.add("waiting");
        const  requestData = JSON.stringify(data);
        displayStatusMessage("wsSendMomentaryToggle SetLimit " + blindOpenFlag.toString());
        socket.send(requestData);
      }
    }

    // Function to display status messages at the bottom
    function displayStatusMessage(message) { //GOOD
        var statusMessageElement = document.getElementById("statusMessage");
        // statusMessageElement.textContent = message;
        console.log(message);
        // Clear the status message after a certain time (e.g., 5 seconds)
        setTimeout(function() {
        statusMessageElement.textContent = "";
        }, 1000);
    }

    // send on/off button  info via WS
    function wsToggleBlinds() { // GOOD

      var toggleButton = document.getElementById("toggleButton");

      // Toggle class 'off' to change background and text color
      toggleButton.classList.toggle("off");
      // Toggle text content between "Open"-1 and "Close"-0
      data["status"]= toggleButton.classList.contains("off") ? 1: 0;
      blindOpenFlag = data["status"];

      toggleButton.textContent = toggleButton.classList.contains("off") ? "Close" : "Open";
      const requestData = JSON.stringify(data)
      console.log(" status is " + requestData);
      socket.send(requestData);
      // Send the status value to the Arduino server
    }

        
    // Function to initiate a factory reset from the URL
    function factoryResetFromURL() {
      const urlParams = new URLSearchParams(window.location.search);
    
      const factoryResetKey = urlParams.get("resetKey");
   
      // Check if the URL contains the factory reset command
      if (factoryResetKey) {
         const data = {
            resetKey: factoryResetKey
         };
   
         const requestData = JSON.stringify(data);
         console.log("factoryResetfromURL: " + requestData);
   
         // Use regular HTTP POST instead of WebSocket
         fetch("/reset", {
            method: 'POST',
            headers: {
               'Content-Type': 'application/json'
            },
            body: requestData
         })
         .then(response => {
            if (!response.ok) {
               throw new Error('Network response was not ok');
            }
            return response.json();
         })
         .then(responsedata => {
            // Handle the response if needed
            console.log("Factory reset completed successfully");
            // make a call to restart 
            blindName="";
            limitSetupFlag=0;
            blindOpenFlag=0;
            setControlsContainerName();
         })
         .catch(error => {
            console.error('Error during factory reset:', error.message);
         });
      }
   }   
    // Public API
    return {
        
        firstLoad,
        wsUpdateOpenRange,
        wsSendMomentaryToggle,
        wsToggleBlinds,
        factoryResetFromURL
      // Add other public functions...
    };
  })();
  
  // Module for handling UI interactions
const UIModule = (function () {

    // Function to handle toggle button click
    function handleToggleButton() {
        const toggleButton = document.getElementById("toggleButton");
        toggleButton.addEventListener("click", () => {
        BlindsModule.wsToggleBlinds();
        });
    }
 
    // Function to handle momentary button click
    function handleMomentaryButtonClick() {
        const momentaryButton = document.getElementById("momentaryButton");
        momentaryButton.addEventListener("click", () => {
        BlindsModule.wsSendMomentaryToggle();
        });
    }
    
    // Function to handle open range slider change
    function handleOpenRangeSliderChange() {
        const openRangeSlider = document.getElementById("openRangeSlider");
        openRangeSlider.addEventListener("input", (event) => {
          if (BlindsModule.limitSetupFlag <3) {
            alert("Windows limits have to be set first !" + BlindsModule.limitSetupFlag);
            // Prevent the wsUpdateOpenRange function from being called
            event.preventDefault();
            return;
          }
          BlindsModule.wsUpdateOpenRange(openRangeSlider.value);
        });
    }


    return {
        handleMomentaryButtonClick,
        handleOpenRangeSliderChange,
        handleToggleButton
        
    };

  })();
  
  // Usage of the modules
  window.addEventListener('DOMContentLoaded', () => {

      BlindsModule.firstLoad();
      // Other UI interactions...
      UIModule.handleToggleButton();
      UIModule.handleMomentaryButtonClick();
      UIModule.handleOpenRangeSliderChange();
      BlindsModule.factoryResetFromURL();
 
  });
  