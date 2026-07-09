{let ad=document.documentURI, re=/[0-9]\//, ind=ad.search(re); U={srv: ad.substring(0, ind+1), root: ad.substring(ind+1, ad.lastIndexOf("/")-2)}, D={a: [], v: []}};
var a=A=jA=[], L="f", curht;//curht is the i(nteger) D.v[i] which changes on clicking to another div/class

async function req(str, code){
	var rtn, qstr="\1"+str+"\2"+code, rsp=await fetch(U.srv, {method: "put", body: qstr});
	if (code[0]=="s" && rsp.ok) return true;
	if (code[1]=="3") return rsp.text();
	if (code[1]=="4") return rsp.json();
	else return false;
}

function loop(n, inc, nmin, nmax){//nmax is the length of the array NOT l-1
	if (inc<0){if (n+inc>=nmin) return n+inc; else return nmax-1;}
	else if (inc>0){if (n+inc>=nmax) return nmin; else return n+inc;}
	else if (inc==0) return n;
}

function setLayout(l){
	var x=document.querySelectorAll("body>div");
	for (var i=0; i<x.length; i++){
		if (x[i].id[0]==l) x[i].style.display="grid";
		else x[i].removeAttribute("style");
	}
	L=l;
	D.a=document.querySelectorAll(`body>div[id]>div[class]`); 
	D.v=document.querySelectorAll(`body>div[id^="${l}"]>div[class]`);
	 //this last bit pretty flaky with different tables/lists likely to occur
}

/*managedir(){
	reads the children of D.v[1];
	in layout "f":
		if they begin "+" it makes the file, if they begin "-" it destroys it
	in layout "t":
		if they begin "+" it splices into that array position, if "-" it splices out
	this is getting called from nowhere... thing to do is remove all specifics and start with the mechanics of what to do
}*/
async function managedir(){
	var str=`${U.root}shell/setDa1.sh`, dstr=D.v[0].children[0].textContent, cstr="", rstr="", sep="", tstr="", ch=D.v[1].children;
	for (var i=0; i<ch.length; i++){
		tstr=ch[i].textContent;
		if (tstr[0]=="-"){
			sep = rstr.length>0 ? "," : "-d ";
			rstr+=`${sep}${tstr.substring(1)}`;
		}
		else if (tstr[0]=="+"){
			sep = cstr.length>0 ? "," : "-c ";
			cstr+=`${sep}${tstr.substring(1)}`;
		}
		else continue;
	}
	D.v[1].innerHTML="<div></div>";
	D.v[1].insertAdjacentHTML("afterbegin", await req(`${str} -p ${dstr} ${cstr} ${rstr}`, `m3`));
}

function managearr(){
	let ch=D.v[1].children, x=-1;
	for (var i=0; i<ch.length; i++){
		/*if ((x=["+", "-"].indexOf(ch[i].innerHTML[0]))>-1){
			if (x==0){
				var xval = x.trim.length>1 ?
				a.splice(i, 1,)
			}
		}*/
		function parseSignString(str){
			const match=/^\s*([+-])(.*)/.exec(str);
			if (!match) return str;
			return {sign: match[1], remainder: match[2]};
		}
		function parseString(str){
			return parseSignString(str);
		}
		console.log(parseString(ch[i].innerText));
	}
}

/*getdir(){
	in layout "f" this is the function that gets U around the filesystem - using the shell script finds.sh
	it needs some equivalent to do the array in "t" - which it's getting down to but nothing serious at all as yet
}*/
async function getdir(e){
	async function fpop(str){
		D.v[0].children[0].innerHTML=str;
		D.v[1].innerHTML=await req(`${U.root}shell/finds.sh \"${str}\"`, 'm3');
	}
	if (e.button==2 && e.type=="mouseup"){
		var s=window.getSelection(), an=s.anchorNode, t=an.textContent, o=s.anchorOffset, ta=t.split("/"), tl=t.substring(0, o).split("/").length, newstr="", idc=[e.target.closest(`body>div[id]`).id, e.target.closest(`body>div[id]>div[class]`).className];
		if (idc[0][0]=="f"){
			if (idc[1]=="top"){for (var i=0; i<tl; i++){newstr+=`${ta[i]}/`;} fpop(newstr);}
			else if (idc[1]=="left"){
				newstr+=D.v[0].children[0].innerHTML+t;
				if (t[t.length-1]=="/")	fpop(newstr);
				else {
					d0a=D.v[0].textContent.split("/"); 
					if (d0a.includes("media")) console.log(`${newstr} is a file you may want to open as a db file...`);
					else if (d0a.includes("docs")){ //console.log(`${newstr} is a file you may want to open as a json file...`);
						//console.log(await req(`cat ${newstr}`, `m4`));
						await(A2ui("t", newstr));
					}
				}
			}
		}
		else if (idc[0][0]=="t"){
			//console.log(`an(chorNode)=${an}\nt(ext)=${t}\no(ffset)=${o}\nt(ext)a(rray)=${ta}\nt(ext)l(ength)=${tl}\nidc(losest)=${idc}\n`);
			if (idc[1]=="top"){
				a=A;
				for (var i=0; i<tl; i++){
					newstr+=`${ta[i]}/`;
					for (var j=0; j<a.length; j++){
						if (a[j][1]==0){if (a[j][0]==ta[i]){a=a[j][2]; break;}}
					}
				}
				console.log(newstr, JSON.stringify(a));
			}
		} 
	}
}
/*
function ui2a(){
	a=A=[], d=D.v[0].innerText.split("/");
	a.push(JSON.parse(`["${d[0]}",0]`));
	for (var i=1; d[i]!=undefined; i++){
		a[0].push(JSON.parse(`[["${d[i]}",0]]`)); 
		a=a[0][2];
	}
	a[0].push([]);
	ch=D.v[1].children;
	for (var i=0; i<ch.length; i++){
		var type = ch[i].textContent[ch[i].textContent.length-1]=="/" ? 0 : 1;
		var active = ch[i].hasAttribute("style") ? D.v[2].innerHTML : "<div></div>"
		a[0][2].push([ch[i].innerHTML, type, active]);
	}
	console.log(JSON.stringify(A));
}

function a2ui(ar, d, ch){//ar=the array, d=depth of folder, ch=child index of file
	//this is more or less a way to read the written/saved array to the visible surface of the UI...
	var hstr="";
	if (d==undefined) d=1024;
	for (var i=0; ar[0][1]==0 && i<d; i++){
		hstr+=`${ar[0][0]}/`;
		ar=ar[0][2];
	}
	D.v[0].children[0].innerHTML=hstr;
	hstr="";
	if (ch==undefined) ch=ar.length-1;
	for (var i=0; i<ar.length; i++){
		var x = ar[i][0].length==0 ? i : ar[i][0], y = ar[i][1]==0 ? "/" : "", z= i==ch ? ` style="color: #a33;"` : "";
		hstr+=`<div${z}>${x}${y}</div>`;
	}
	D.v[1].innerHTML=hstr;
	D.v[2].innerHTML = ar[ch][1]==1 ? ar[ch][2] : "<div></div>";
}
*/

async function A2ui(l, file){//having imported jA[0], and a=A=jA[0], read the root directory into gui - as if you pressed the empty D.v[0] in f...
	//and it looks like this needs to be done for each different type of document... for now, each layout using a different json pattern - t being at its simplest [["",0,[["",1,"<div></div>"]]]] - i need to specify here that if the document is made and didn't previously exist, the root array begins as that
	//as soon as i've done this one - before going on to how text/virtual fs files get saved, i'll work out how the server is going to return the size of the file sound/made (so the difference between the two can be distinguished... or should the script for the js make a file with a template js array in it that just writes an empty page in any/every type? ones for a music database, a small image database and an image/text editor
	//fact, this IS js - you know what layout you're in 
	//, tail=file.substring(file.indexOf("/ht/")+4);
	
	let x=0, hstr="", top=""; try{x=jA.push(await req(`cat ${file}`, `m4`)); a=A=jA[x-1];} catch(e){x=jA.push([{name:"", type: 0, data: [{name: "", type: 1, data: "<div></div>"}]}]); a=A=jA[x-1];}
	//console.log(JSON.stringify(a), JSON.stringify(A), JSON.stringify(jA));
	//the assumption here is that a complex array has to exist because we have a complex DOM
		//better to assume - as usual - as always - that a[] needs to be reset: 
			//[["",1,"<div></div>"]] is fine as a definition of the bottom two divs, and there is no need to fill out the top if you use the filename as a constant:
			
			
	top = a.length!=1 ? "" : `${a[0].name.length==0 ? "0" : a[0].name}/`;
	a=a[0].data;
	for (var i=0; i<a.length; i++){
		let isdir="", name="";
		if (a[i].type==0) isdir="/";
		if (a[i].name.length==0) name=`${i}`; else name=a[i].name;
		hstr+=`<div>${name}${isdir}</div>`;
	}
	setLayout("t");
	D.v[0].children[0].innerHTML=top;
	D.v[1].innerHTML=hstr;
}

function m(e){
	getdir(e);
}

function kd(e){
	if (e.ctrlKey){
		switch (e.key){
			case "f": case "t": case "a": case "i": case "m": e.preventDefault(); setLayout(e.key); break;//v is the character for 'paste'!!
			case "Enter":{
				e.preventDefault()
				console.log(window.getSelection());
				break;
			}
		}
	}
}

//var editor=document.getElementById('my-editor');

function writeA(e){
	//const time = new Date().toLocaleTimeString();
  //console.log(`[${time}] ${e.target.tagName} (${e.target.textContent.trim()}) lost consciousness...`);
  if (e.target==D.v[0]) console.log(e.target.textContent.split("/"));
  if (e.target==D.v[1]) console.log(e.target.innerHTML);
  if (e.target==D.v[2]) console.log(e.target.innerHTML);
}

function paste(e){
	e.preventDefault();
	var dp=e.clipboardData.getData("text/plain"),dh=e.clipboardData.getData("text/HTML")
	console.log(dp); console.log(dh);
}


window.addEventListener("load", function(){setLayout("f");})
window.addEventListener("mousedown", m);
window.addEventListener("mouseup", m);
window.addEventListener("focusout", writeA);
window.addEventListener("focusin", writeA);
window.addEventListener("keydown", kd);
window.addEventListener("paste", paste);

window.addEventListener("contextmenu", function(){event.preventDefault();})
window.addEventListener("dragstart", function(){event.preventDefault();})
