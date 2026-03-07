import type { MouseEvent } from "react";
import { useState } from "react";

// interface defines the structure of an object
interface Props{
  items: string[];
  heading: string;
  // onSelectItem (conventional name) is a function that takes a string and returns void (i.e. the return statement is just 'return')
  // in typescript, we need to specify the type of parameters.
  onSelectItem: (item: string) => void;
}

function ListGroup(props: Props) { // <-- props is special. We give it type Props (the interface above)

  // useState is a 'hook' which stops selectedIndex getting reset every render (selectedIndex is local to this component)
  // useState returns an array [variable, function that changes it], which we destructure with the below cnovention
  // each component has its own state, so the two lists on the webpage allow selection of items independently
  const [selectedIndex, setSelectedIndex] = useState(-1);

  // we use onClick in react, referencing handleClick later
  // event is special. Below, we need to specify event's type. We import this type (MouseEvent) from react
  const handleClick = (event: MouseEvent) => {
    console.log(`${event.clientX}, ${event.clientY}`);
  };

  const listItems = props.items.map((item: string, index: number) => (
    <li
      key={index}
      className={selectedIndex === index ? 'list-group-item active' : 'list-group-item'}
      onClick={(event) => {
        handleClick(event);
        setSelectedIndex(index);
        props.onSelectItem(props.items[index]);
      }}
    >
      {item}
    </li>
  ));

  return (
    <>
      <h1>{props.heading}</h1>
      {/* Below: the guard operator is a common techniue used to conditionally render content. Here's an example */}
      {props.items.length === 0 && <p>No items found</p>}
      <ul className="list-group">
        {/* class is a javascript keyword so we use className when writing HTML with react */}
        {listItems}
      </ul>
    </>
  );
}

export default ListGroup;
