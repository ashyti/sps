# Smart Power Switch (SPS) based in RT-Thread

The SPS handles the power on an array of devices. A common use is a testing farm where the SPS is able
to detect unresponsive machines and send feedback to the user. On the other hand, the user can decide
to power on and/or off the machines connected.



## SPS architecture

The architecture of the Smart Power Switch and some external components are described as following:

![SPS diagram](/images/sps_diagram.png)

- The SPS: is the controller unit of the system. Allowing users to easily interact with the testing farm.
- The host: represents the interface with the user (a Raspberry Pi is shown as an example). To avoid using any external hardware, the host device is simulated with different threads inside the RT-Thread OS. 
  The host device serves two purposes.
  - Displays the power status of all target machines to the user.
  - Allows the user to power ON/OFF any target machine.
- The Target devices: represents a set of servers within a testing farm. Each device is simulated by a periodic task.


## Inputs and outputs

- Inputs
  - IRQ_IN: Triggered by the host to power on/off a device in the array.
- Outpus
  - GPIO[1..4] : Used to power ON/OFF the machines in the array. To the Enable pins of the power switch.
  - IRQ_OUT: The SPS signals the host that the status of a machine has changed.
- Input/Output
  - Ethernet: Pinging status of all machines in the array
  - I2C
    - After IRQ_IN : Receives commands from the host to power ON or OFF a machine.
	- After IRQ_OUT: Sends an array of status (on/off) to the host of all machines in the array.

## Tasks

![SPS tasks](/images/sps_tasks.png)