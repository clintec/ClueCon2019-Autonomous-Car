# ClueCon2019-Autonomous-Car

This project is a customization of the UCTRONICS Smart Robot Car Demo.
Web: http://www.uctronics.com

It was winning customized for ClueCon 2019's Autonomous Car Competition.
The customizations from the stock application are listed below.

     Team Straight Outta ClueCon Customizations  (Chris Cline & Jeff Benson)
 ---------------------------------------------------------------------------------------------------------
 *   Defined static variables for speeds and delays.
 *   Customized the forward speed to 255 (max speed)
 *   Customized long delay between ultrasound scanning to be shorter.
 *   Commented out #2 motors.
 *   Informed the car to not stop at the start of the turn loop, but rather just slow down. 
 *   If the car determines that the distance between both left and right are similar, stop the car from moving forward.
 *   If the car determines that the distance between both left and right are similar, double the backup distance.
 *   Extend the distance used to scan for object detection from about 4" to about 10".
 ---------------------------------------------------------------------------------------------------------

