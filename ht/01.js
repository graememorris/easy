{let ad=document.documentURI, re=/[0-9]\//, ind=ad.search(re); U={srv: ad.substring(0, ind+1), root: ad.substring(ind+1, ad.lastIndexOf("/")-2)}, D={a: [], v: []}};
var a, A, jA=[], L="f", curht;//curht is the i(nteger) D.v[i] which changes on clicking to another div/class

async function req(str, code){
	var rtn, qstr="\1"+str+"\2"+code, rsp=await fetch(U.srv, {method: "put", body: qstr});
	if (code[0]=="s" && rsp.ok) return true;
	if (code[1]=="3") return rsp.text();
	if (code[1]=="4"){var x=0; try{x=jA.push(await rsp.json()); a=A=jA[x-1];} catch(e){x=jA.push([["", 0, [["", 1, "<div></div>"]]]]); a=A=jA[x-1];}}
	else return false;
}

function loop(n, inc, nmin, nmax){//nmax is the length of the array NOT l-1
	if (inc<0){if (n+inc>=nmin) return n+inc; else return nmax-1;}
	else if (inc>0){if (n+inc>=nmax) return nmin; else return n+inc;}
	else if (inc==0) return n;
}
/*
async function init(l){
	var ad=document.documentURI, re=/[0-9]\//, ind=ad.search(re);
	//U.srv=ad.substring(0, ind+1); U.root=ad.substring(ind+1, ad.lastIndexOf("/")+1);
	//await req(`cat ${U.root}09tmps.json`, `m4`);
	//for (var i=0; i<A.length; i++){document.body.insertAdjacentHTML("beforeend", A[i]);}
	setLayout(l);
	//i have no idea why this is working - setLayout's not returning anything - there's no suspicion of a callback - yet THIS function is letting setLayout finish without moving on...(??????????)
	//it is an ideal place to define eventListeners such that they'll work different ways for different layouts...
		//i wonder if maybe setLayout somehow BECOMES asynchronous if used in an asynchronous function?
	//for (var i=0; i<D.a.length; i++){D.a[i].addEventListener("mouseup", dState);}
}
*/
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
	if they begin "+" it makes the file, if they begin "-" it destroys it
	it needs to apply also to the array - so + or - causes a.splice() (preferably in the right place)
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

async function docint(path, type){
	await req(`${U.root}shell/newfile01.sh ${path} ${type}`, `m4`);
	setLayout(type);
	/*
	a=A=jA[jA.push(await req(`cat ${path}`, `m4`))-1]
	*/
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
					else if (d0a.includes("docs")) console.log(`${newstr} is a file you may want to open as a json file...`);
				}
			}
		}
		else if (idc[0][0]=="t"){
			if (idc[1]=="top"){
				for (var i=0; i<tl; i++) newstr+=`${ta[i]}/`;
				D.v[0].children[0].innerHTML=newstr;
			}
		} 
	}
}

function dState(e){
	//console.log(curht, e.target.closest(`body>div[id]>div[class]`), curht==D.v[0]);
	var i=0; for (; i<D.v.length; i++){if (curht==D.v[i]) break;} console.log(i, D.v[i]);
	curht=e.target.closest(`body>div[id]>div[class]`);
}

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

function paste(e){
	e.preventDefault();
	var dp=e.clipboardData.getData("text/plain"),dh=e.clipboardData.getData("text/HTML")
	console.log(dp); console.log(dh);
	/*
D.a[5].addEventListener('paste', (event) => {
    // 1. Prevent the default browser rich-text paste action
    event.preventDefault();

    // 2. Fetch data from clipboard strictly as plain text (drops all HTML/CSS)
    let rawText = (event.clipboardData || window.clipboardData).getData('text/plain');
    
		console.log(rawText)
    
    

    // 3. Purge all tabs (\t), newlines (\n), and carriage returns (\r)
    // This replaces them with a single standard space
    rawText = rawText.replace(/[\t\n\r]+/g, ' ');

		console.log(rawText)
    // 4. Safely insert the fully sanitized string at the current cursor position
    const selection = window.getSelection();
    if (!selection.rangeCount) return;
    
    selection.deleteFromDocument();
    selection.getRangeAt(0).insertNode(document.createTextNode(rawText));
    
    // 5. Collapse the cursor to the end of the newly pasted text
    selection.collapseToEnd();
    */
}


window.addEventListener("load", function(){setLayout("f");})
window.addEventListener("mousedown", m);
window.addEventListener("mouseup", m);
window.addEventListener("keydown", kd);
window.addEventListener("paste", paste);

window.addEventListener("contextmenu", function(){event.preventDefault();})
window.addEventListener("dragstart", function(){event.preventDefault();})
