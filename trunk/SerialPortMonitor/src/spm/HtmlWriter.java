package spm;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;

public class HtmlWriter {

	BufferedWriter fw;
	boolean odd = true;
	int index;
	
	public HtmlWriter(String fileAbsolutePath) throws IOException {
		fw = new BufferedWriter(new FileWriter(fileAbsolutePath, true));
		fw.append("<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\">\n");
	}
	
	public void append(byte[] data) throws IOException {
		int width = toInt(data[1]) != 0 ? 80 : toInt(data[2]) * 80 / 255;
		fw.append("<div style=\"width: " + width + "px;\" class=\"" + (odd ? "odd" : "even") + "\">");
		
		if (odd) {
			fw.append("<span class=\"index\">" + String.format("%02d", index++) + "</span>");
		}
		
		if (toInt(data[0]) > 0) {
			fw.append("<span class=\"first\">" + toInt(data[0]) + "</span>");	
		}
		
		if (toInt(data[1]) > 0) {
			fw.append("<span class=\"second\">" + toInt(data[1]) + "</span>");	
		}
		
		fw.append("<span class=\"third\">" + toInt(data[2]) + "</span>");
		fw.append("</div>");
		
		odd = !odd;
	}
	
	public void startRow() throws IOException {
		fw.append("<div class=\"row\">\n");
		odd = true;
		index = 0;
	}
	
	public void endRow() throws IOException {
		fw.append("\n</div>\n");
		fw.flush();
	}
	
	public void close() throws IOException {
		fw.close();
	}
	
	private int toInt(byte b) {
		return (int) (b & 0xFF);
	}
}
