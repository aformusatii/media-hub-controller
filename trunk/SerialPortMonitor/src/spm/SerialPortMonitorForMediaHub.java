package spm;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import jssc.SerialPort;
import jssc.SerialPortEvent;
import jssc.SerialPortEventListener;
import jssc.SerialPortException;
import jssc.SerialPortList;

public class SerialPortMonitorForMediaHub implements SerialPortEventListener {

	private SerialPort serialPort;
	
	/**
	 * @param args
	 * @throws SerialPortException 
	 * @throws IOException 
	 */
	public static void main(String[] args) throws SerialPortException, IOException {
		new SerialPortMonitorForMediaHub().start();
	}
	
	public void start() throws SerialPortException, IOException {
		int portIndex = 0;
		boolean loop = true;
		
		String[] portNames = SerialPortList.getPortNames();

		for (String portName : portNames) {
			System.out.println("Port Name: " + portName);
		}
		
		
		serialPort = new SerialPort(portNames[portIndex]);
		serialPort.openPort();		
		serialPort.setParams(SerialPort.BAUDRATE_19200, 
				             SerialPort.DATABITS_8, 
				             SerialPort.STOPBITS_1, 
				             SerialPort.PARITY_NONE);
		serialPort.addEventListener(this);
		
		while (loop) {
			String line = readLine(">: ");
			
			if ("exit".equalsIgnoreCase(line)) {
				break;
			}
			
			serialPort.writeByte((byte) 0x00);
		};
		
		serialPort.closePort();	
	}

	
	@Override
	public synchronized void serialEvent(SerialPortEvent serialPortEvent) {
		//System.out.println(serialPortEvent.getEventType());
		
		try {
			byte[] data = serialPort.readBytes();
			
			for (byte b : data) {

			    //String s2 = String.format("%8s", Integer.toBinaryString(b2 & 0xFF)).replace(' ', '0');
			    //System.out.println(s2);
				
				System.out.print(new String(new byte[] {b}));
			}
			
			//String str = new String(data);
			//System.out.println(str);
			
		} catch (SerialPortException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	private String readLine(String format, Object... args) throws IOException {
	    if (System.console() != null) {
	        return System.console().readLine(format, args);
	    }
	    
	    System.out.print(String.format(format, args));
	    BufferedReader reader = new BufferedReader(new InputStreamReader(
	            System.in));
	    return reader.readLine();
	}

}