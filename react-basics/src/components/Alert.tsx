import type {ReactNode} from "react";

// children is a special attribute for this interface with type ReactNode. It takes the children of <Alert></Alert> (see App.tsx)
// type ReactNode means we can pass HTML code into <Alert></Alert> (also see App.tsx)
interface Props{
  dismissible: boolean;
  onClose: () => void;
  children: ReactNode;
};

const Alert = (props: Props) => {
  return (
    <>
      {/* these special classes can all be found in the components section of the react documentation */}
      <div className="alert alert-success alert-dismissible">{props.children}{
        props.dismissible && <button type="button" className="btn-close" aria-label="Close" onClick={props.onClose}></button>
      }</div>
    </>
  );
}

export default Alert;