#ifndef WEBPAGES_H
#define WEBPAGES_H

const char* PROGMEM html_root = R"=====(

<html>
    <head>
        <title>Ledgend</title>
        <meta charset="UTF-8">
    </head>

    <body>
        <input type="button" id="refresh" value="Refresh">

        <div id="networks">
        </div>
        <label id="warning" style="color: red;"></label>

        <input type="text" id="password" placeholder="Password">
        <input type="button" id="connect" value="Connect">


        <script>
            let refresh_e = document.getElementById("refresh");
            let networks_table_e = document.getElementById("networks");
            let warning_e = document.getElementById("warning");
            let password_e = document.getElementById("password");
            let connect_e = document.getElementById("connect");

            let connected = true;
            let connected_network_info = "";

            refresh_e.addEventListener("click", refreshNetworks);
            connect_e.addEventListener("click", () => {
                isConnected(attemptConnection);
            });

            isConnected(refreshNetworks);

            setInterval(() => {
                isConnected();
            }, 1000);

            function isConnected(cb) {
                let xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function() {
                    if (this.readyState == 4 && this.status == 200) {
                        if ( this.responseText == "false" ) {
                            connected = false;
                        } else {
                            connected = true;
                            connected_network_info = this.responseText;
                            showConnectionInfo();

                            // show an empty table
                            showTable([]);
                        }

                        if ( cb ) {
                            cb();
                        }
                    }
                }
                xhttp.open("GET", "status", true);
                xhttp.send();
            }

            function attemptConnection() {
                showWarning("Connecting!")

                if ( connected ) {
                    showConnectionInfo();
                    return;
                }

                let network_radio = document.querySelector(
                    "input[name=network_group]:checked"
                );
                let checked_network = network_radio ? network_radio.value : "";

                if ( checked_network == "" ) {
                    showWarning("No network chosen!");
                    return;
                }

                let xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function() {
                    if (this.readyState == 4 && this.status == 200) {
                        connected_network_info = this.responseText;
                        hideWarning();
                    }
                }
                xhttp.open(
                    "GET",
                    "connect?SSID="+checked_network+"&PASS="+password_e.value,
                    true
                );
                xhttp.send();
            }


            // networks is an array of arrays
            // each nested array contains SSID and RSSI of a network
            function showTable(networks) {
                let HTMLString = "";
                networks.forEach((network) => {
                    HTMLString += `
                        <input type="radio" name="network_group"
                            value="${network[0]}">
                        <label for="${network[0]}">
                            (${network[1]}) ${network[0]}
                        </label>
                        <br>
                    `
                });
                networks_table_e.innerHTML = HTMLString;
            }

            function refreshNetworks() {
                showWarning("Refreshing!");

                if ( connected ) {
                    showConnectionInfo();
                    return;
                }


                let xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function() {
                    if (this.readyState == 4 && this.status == 200) {
                        let json = JSON.parse(this.responseText);
                        showTable(json.networks);
                        hideWarning();
                    }
                };
                xhttp.open("GET", "networks", true);
                xhttp.send();
            }

            function showWarning(message) {
                warning_e.style.display = "block";
                warning_e.innerHTML = message;
            }

            function hideWarning() {
                warning_e.style.display = "none";
            }

            function showConnectionInfo() {
                showWarning("Connected to " + connected_network_info);
            }

        </script>
    </body>
</html>



)=====";

#endif
