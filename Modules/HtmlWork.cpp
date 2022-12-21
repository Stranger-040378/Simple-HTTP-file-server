#include "HtmlWork.hpp"

using namespace std;

string GetHtmlHeaderT(vector <string> Caption)
{
	string RetVal =
	
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\r\n"
	"<HTML><HEAD><META http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n"
	"<META NAME=\"GENERATOR\" content=\"Only by hands\"><TITLE>File server test</TITLE>\r\n"
	"<style type=\"text/css\">\r\n"
	"\tbody { margin: 0px; padding: 0px; } \r\n"
	"\ttable { margin: auto; width: 80%; table-layout: auto; border: 1px solid gray; border-spacing: 0px; border-collapse: collapse; } \r\n"
	"\ttd, th { margin: 0px; padding: 5px 10px; vertical-align: middle; text-align: center; border: 1px solid gray; font-family: sans-serif; font-size: medium; color: black; } \r\n"
	"</style>\r\n</HEAD>\r\n<body><table><tbody>\r\n";
	
	RetVal += "<tr>";
	for (auto & Str : Caption)
		RetVal += "<td>" + Str + "</td>";
	
	RetVal += "</tr>\r\n";

	return RetVal;
}

string GetHtmlFooterT()
{
	string RetVal = "</tbody></table></body></HTML>\r\n"; return RetVal;
}

string GetTableRow(vector <string> Data)
{
	string RetVal = "<tr>";
	
	for (auto & Str : Data)
		RetVal += "<td>" + Str + "</td>";
	
	RetVal += "</tr>\r\n";
	
	return RetVal; 
}

string GetFileSubmitForm(string InUrl)
{
	string RetVal =	
		"<FORM ENCTYPE=\"multipart/form-data\" ACTION=\"" + InUrl + "\" METHOD=POST>\r\n" +
		"\t<INPUT NAME=\"userfile1\" TYPE=\"file\">\r\n" +
		"\t<INPUT TYPE=\"submit\" VALUE=\"Send File\">\r\n" +
		"</FORM>\r\n";
	
	return RetVal;
}

string GetHRef(string InUrl, string Text)
{
	return "<a href=\"" + InUrl + "\">" + Text + "</a>";
}
