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

## Mailboxes
As everything within this project is simulated, in contrast to using real I2C lines, GPIO lines or ethernet lines, in order to synchronize threads and send messages between them, mails and mailboxes are used.
Each mail can hold up to 4 bytes (32 bits) of data or a pointer to the address of the data. Mails are sent to a queued mailbox and can be read later by any other thread which has access to the mailbox, mails are also read sequentially in queue.
![Mailbox](/images/mailbox.png)

### IRQ_IN
When the user wants to set the status of a target, either by powering it on, or shutting it down, a mail containing a 4 bit data is sent. The status of all four targets are represented in this mail, either by a 0 (Shutdown) or a 1 (Power on).

### GPIO
From the previous email, the SPS decodes the desired status for each target and will send individual mails to each of them with the power command. There are individual mailboxes shared between the SPS and each of the targets, so each target is separated from the others and only knows about its own status.
![GPIO Mailbox](/images/gpio_mailbox)

### Ping
A mail  containing the data 0x01, is sent to every target in the system and waits for the response. If a target is running, it will return a mail containing the data of its internal address, either  0b0001, 0b0010, 0b0100 or 0b1000, depending on which target is responding. This will help the SPS determine who is sending the mail.
![Ping Mailbox](/images/ping_mailbox)
