import type { ReactNode } from "react";

interface Props{
  children: ReactNode;
  colour?: string; // Not used here but ? means that this property is OPTIONAL
  onClick: () => void;
}

function BSButton(props: Props){ // Can destructure the interface (and thus use default values)
  return(
    <button type="button" className={`btn btn-${props.colour}`} onClick={props.onClick}>{props.children}</button>
  );
}

export default BSButton;